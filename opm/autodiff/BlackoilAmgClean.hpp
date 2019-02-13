/*
  Copyright 2017 Dr. Blatt - HPC-Simulation-Software & Services

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef OPM_AMGCLEAN_HEADER_INCLUDED
#define OPM_AMGCLEAN_HEADER_INCLUDED

#include <ewoms/linear/matrixblock.hh>
#include <opm/autodiff/ParallelOverlappingILU0.hpp>
#include <opm/autodiff/FlowLinearSolverParameters.hpp>
#include <opm/autodiff/CPRPreconditioner.hpp>
#include <dune/istl/paamg/twolevelmethod.hh>
#include <dune/istl/paamg/aggregates.hh>
#include <dune/istl/bvector.hh>
#include <dune/istl/bcrsmatrix.hh>
#include <dune/istl/preconditioners.hh>
#include <dune/istl/schwarz.hh>
#include <dune/istl/operators.hh>
#include <dune/istl/scalarproducts.hh>
#include <dune/common/fvector.hh>
#include <dune/common/fmatrix.hh>
namespace Dune
{
  namespace Amg
  {
    template<class M, class Norm>
    class UnSymmetricCriterion;
  }
}

namespace Dune
{

  template <class Scalar, int n, int m>
  class MatrixBlock;

}

namespace Opm
{

  namespace Detail
  {
    template<class Operator, class Criterion, class Communication, std::size_t COMPONENT_INDEX>
    class OneComponentAggregationLevelTransferPolicy;


    /**
     * @brief A policy class for solving the coarse level system using one step of AMG.
     * @tparam O The type of the linear operator used.
     * @tparam S The type of the smoother used in AMG.
     * @tparam C The type of the crition used for the aggregation within AMG.
     * @tparam C1 The type of the information about the communication. Either
     *            Dune::OwnerOverlapCopyCommunication or Dune::SequentialInformation.
     */
    template<class O, class S, class C, class P>
    class OneStepAMGCoarseSolverPolicyNoSolve
    {
    public:
      typedef P LevelTransferPolicy;
      /** @brief The type of the linear operator used. */
      typedef O Operator;
      /** @brief The type of the range and domain of the operator. */
      typedef typename O::range_type X;
      /** @brief The type of the crition used for the aggregation within AMG.*/
      typedef C Criterion;
      /** @brief The type of the communication used for AMG.*/
      typedef typename P::ParallelInformation Communication;
      /** @brief The type of the smoother used in AMG. */
      typedef S Smoother;
      /** @brief The type of the arguments used for constructing the smoother. */
      typedef typename Dune::Amg::SmootherTraits<S>::Arguments SmootherArgs;
      /** @brief The type of the AMG construct on the coarse level.*/
      typedef Dune::Amg::AMG<Operator,X,Smoother,Communication> AMGType;
      /**
       * @brief Constructs the coarse solver policy.
       * @param args The arguments used for constructing the smoother.
       * @param c The crition used for the aggregation within AMG.
       */
      OneStepAMGCoarseSolverPolicyNoSolve(const CPRParameter* param, const SmootherArgs& args, const Criterion& c)
        : param_(param), smootherArgs_(args), criterion_(c)
      {}
      /** @brief Copy constructor. */
      OneStepAMGCoarseSolverPolicyNoSolve(const OneStepAMGCoarseSolverPolicyNoSolve& other)
        : param_(other.param_), coarseOperator_(other.coarseOperator_), smootherArgs_(other.smootherArgs_),
          criterion_(other.criterion_)
      {}
    private:
      /**
       * @brief A wrapper that makes an inverse operator out of AMG.
       *
       * The operator will use one step of AMG to approximately solve
       * the coarse level system.
       */
      struct AMGInverseOperator : public Dune::InverseOperator<X,X>
      {
        AMGInverseOperator(const CPRParameter* param,
                           const typename AMGType::Operator& op,
                           const Criterion& crit,
                           const typename AMGType::SmootherArgs& args,
                           const Communication& comm)
	  : param_(param), amg_(), op_(op), comm_(comm)
        {
	  amg_.reset(new AMGType(op, crit,args, comm));
        }


        void apply(X& x, X& b, double reduction, Dune::InverseOperatorResult& res)
        {
	  DUNE_UNUSED_PARAMETER(reduction);
	  DUNE_UNUSED_PARAMETER(res);

	  using Chooser = Dune::ScalarProductChooser<X,Communication,AMGType::category>;
	  auto sp = Chooser::construct(comm_);
	  Dune::Preconditioner<X,X>* prec = amg_.get();
	  // Linear solver parameters
	  const double tolerance = param_->cpr_solver_tol_;
	  const int maxit        = param_->cpr_max_iter_;
	  const int verbosity    = ( param_->cpr_solver_verbose_ &&
				     comm_.communicator().rank()==0 ) ? 1 : 0;
	  if ( param_->cpr_ell_solvetype_ == 0 )
            {
	      // Category of preconditioner will be checked at compile time. Therefore we need
	      // to cast to the derived class
	      Dune::BiCGSTABSolver<X> solver(const_cast<typename AMGType::Operator&>(op_), *sp,
					     reinterpret_cast<AMGType&>(*prec),
					     tolerance, maxit, verbosity);
	      solver.apply(x,b,res);
 
            }
	  else if (param_->cpr_ell_solvetype_ == 1)
            {
	      // Category of preconditioner will be checked at compile time. Therefore we need
	      // to cast to the derived class
	      Dune::CGSolver<X> solver(const_cast<typename AMGType::Operator&>(op_), *sp,
				       reinterpret_cast<AMGType&>(*prec),
				       tolerance, maxit, verbosity);
	      solver.apply(x,b,res);
            }
	  else
	    {
	      // X v(x);
	      // prec->pre(x,b);
	      // op_->applyscaleadd(-1,x,b);
	      // v = 0;
	      // prec->apply(v,b);
	      // x += v; 
	      // op_->applyscaleadd(-1,x,b);
	      // prec->post(x);
	      Dune::LoopSolver<X> solver(const_cast<typename AMGType::Operator&>(op_), *sp,
				       reinterpret_cast<AMGType&>(*prec),
				       tolerance, maxit, verbosity);
	      solver.apply(x,b,res);
	    }
	  delete sp;
        }

        void apply(X& x, X& b, Dune::InverseOperatorResult& res)
        {
	  return apply(x,b,1e-8,res);
        }

        ~AMGInverseOperator()
        {}
        AMGInverseOperator(const AMGInverseOperator& other)
	  : x_(other.x_), amg_(other.amg_)
        {
        }
      private:
        const CPRParameter* param_;
        X x_;
        std::unique_ptr<AMGType> amg_;
        const typename AMGType::Operator& op_;
        const Communication& comm_;
      };

    public:
      /** @brief The type of solver constructed for the coarse level. */
      typedef AMGInverseOperator CoarseLevelSolver;

      /**
       * @brief Constructs a coarse level solver.
       *
       * @param transferPolicy The policy describing the transfer between levels.
       * @return A pointer to the constructed coarse level solver.
       */
      template<class LTP>
      CoarseLevelSolver* createCoarseLevelSolver(LTP& transferPolicy)
      {
        coarseOperator_=transferPolicy.getCoarseLevelOperator();
        const LevelTransferPolicy& transfer =
	  reinterpret_cast<const LevelTransferPolicy&>(transferPolicy);
        AMGInverseOperator* inv = new AMGInverseOperator(param_,
                                                         *coarseOperator_,
                                                         criterion_,
                                                         smootherArgs_,
                                                         transfer.getCoarseLevelCommunication());

        return inv; //std::shared_ptr<InverseOperator<X,X> >(inv);

      }

    private:
      /** @brief The coarse level operator. */
      std::shared_ptr<Operator> coarseOperator_;
      /** @brief The parameters for the CPR preconditioner. */
      const CPRParameter* param_;
      /** @brief The arguments used to construct the smoother. */
      SmootherArgs smootherArgs_;
      /** @brief The coarsening criterion. */
      Criterion criterion_;
    };

          
  } // end namespace Detail

 
  /**
   * \brief An algebraic twolevel or multigrid approach for solving blackoil (supports CPR with and without AMG)
   *
   * This preconditioner first decouples the component used for coarsening using a simple scaling
   * approach (e.g. Scheichl, Masson 2013,\see scaleMatrixDRS). Then it constructs the first
   * coarse level system, either by simply extracting the coupling between the components at COMPONENT_INDEX
   * in the matrix blocks or by extracting them and applying aggregation to them directly. This coarse level
   * can be solved either by AMG or by ILU. The preconditioner is configured using CPRParameter.
   * \tparam O The type of the operator (encapsulating a BCRSMatrix).
   * \tparam S The type of the smoother.
   * \tparam C The type of coarsening criterion to use.
   * \tparam P The type of the class describing the parallelization.
   * \tparam COMPONENT_INDEX The index of the component to use for coarsening (usually the pressure).
   */
  template<typename O, typename S, typename C,
	   typename P, std::size_t COMPONENT_INDEX>
  class BlackoilAmgClean
    : public Dune::Preconditioner<typename O::domain_type, typename O::range_type>
  {
  public:
    /** \brief The type of the operator (encapsulating a BCRSMatrix). */
    using Operator = O;
    /** \brief The type of coarsening criterion to use. */
    using Criterion = C;
    /** \brief The type of the class describing the parallelization. */
    using Communication = P;
    /** \brief The type of the smoother. */
    using Smoother = S;
    /** \brief The type of the smoother arguments for construction. */
    using SmootherArgs   = typename Dune::Amg::SmootherTraits<Smoother>::Arguments;

  protected:
    using Matrix = typename Operator::matrix_type;
    using CoarseOperator = typename Detail::ScalarType<Operator>::value;
    using CoarseSmoother = typename Detail::ScalarType<Smoother>::value;
    using FineCriterion  =
      typename Detail::OneComponentCriterionType<Criterion,COMPONENT_INDEX>::value;
    using CoarseCriterion =  typename Detail::ScalarType<Criterion>::value;
    using LevelTransferPolicy =
      OneComponentAggregationLevelTransferPolicy<Operator,
						 FineCriterion,
						 Communication,
						 COMPONENT_INDEX>;
    using CoarseSolverPolicy   =
      Detail::OneStepAMGCoarseSolverPolicyNoSolve<CoarseOperator,
						  CoarseSmoother,
						  CoarseCriterion,
						  LevelTransferPolicy>;
    using TwoLevelMethod =
      Dune::Amg::TwoLevelMethod<Operator,
				CoarseSolverPolicy,
				Smoother>;
  public:
    // define the category
    enum {
      //! \brief The category the precondtioner is part of.
      category = Operator::category
    };
    /**
     * \brief Constructor.
     * \param param The parameters used for configuring the solver.
     * \param fineOperator The operator of the fine level.
     * \param criterion The criterion describing the coarsening approach.
     * \param smargs The arguments for constructing the smoother.
     * \param comm The information about the parallelization.
     */
    BlackoilAmgClean(const CPRParameter& param,
		const typename TwoLevelMethod::FineDomainType& weights,
                const Operator& fineOperator, const Criterion& criterion,
                const SmootherArgs& smargs, const Communication& comm)
      : param_(param),
	weights_(weights),
	scaledMatrixOperator_(Detail::scaleMatrixDRS(fineOperator, comm,
						     COMPONENT_INDEX, weights, param)),
	smoother_(Detail::constructSmoother<Smoother>(std::get<1>(scaledMatrixOperator_),
						      smargs, comm)),
	levelTransferPolicy_(criterion, comm, param.cpr_pressure_aggregation_),
	coarseSolverPolicy_(&param, smargs, criterion),
	twoLevelMethod_(std::get<1>(scaledMatrixOperator_), smoother_,
			levelTransferPolicy_,
			coarseSolverPolicy_, 0, 1)
    {}

    void pre(typename TwoLevelMethod::FineDomainType& x,
             typename TwoLevelMethod::FineRangeType& b)
    {
      twoLevelMethod_.pre(x,b);
    }

    void post(typename TwoLevelMethod::FineDomainType& x)
    {
      twoLevelMethod_.post(x);
    }

    void apply(typename TwoLevelMethod::FineDomainType& v,
               const typename TwoLevelMethod::FineRangeType& d)
    {
      auto scaledD = d;
      Detail::scaleVectorDRS(scaledD, COMPONENT_INDEX, param_, weights_);
      twoLevelMethod_.apply(v, scaledD);
    }
  private:
    const CPRParameter& param_;
    const typename TwoLevelMethod::FineDomainType& weights_;
    std::tuple<std::unique_ptr<Matrix>, Operator> scaledMatrixOperator_;
    std::shared_ptr<Smoother> smoother_;
    LevelTransferPolicy levelTransferPolicy_;
    CoarseSolverPolicy coarseSolverPolicy_;
    TwoLevelMethod twoLevelMethod_;
    //BlockVector weights_;
  };

} // end namespace Opm
#endif

# ifndef CPPAD_LOCAL_OPTIMIZE_GET_OP_USAGE_HPP
# define CPPAD_LOCAL_OPTIMIZE_GET_OP_USAGE_HPP

/* --------------------------------------------------------------------------
CppAD: C++ Algorithmic Differentiation: Copyright (C) 2003-18 Bradley M. Bell

CppAD is distributed under multiple licenses. This distribution is under
the terms of the
                    Eclipse Public License Version 1.0.

A copy of this license is included in the COPYING file of this distribution.
Please visit http://www.coin-or.org/CppAD/ for information on other licenses.
-------------------------------------------------------------------------- */
/*!
\file get_cexp_info.hpp
Create operator information tables
*/

# include <cppad/local/optimize/cexp_info.hpp>
# include <cppad/local/optimize/usage.hpp>

// BEGIN_CPPAD_LOCAL_OPTIMIZE_NAMESPACE
namespace CppAD { namespace local { namespace optimize {

/// Is this an addition or subtraction operator
inline bool op_add_or_sub(
	OpCode op ///< operator we are checking
)
{	bool result;
	switch(op)
	{
		case AddpvOp:
		case AddvvOp:
		case SubpvOp:
		case SubvpOp:
		case SubvvOp:
		result = true;
		break;

		default:
		result = false;
		break;
	}
	return result;
}

/*!
Increarse argument usage and propagate cexp_set from result to argument.

\param play
is the player for the old operation sequence.

\param sum_result
is result an addition or subtraction operator (passed for speed so
do not need to call op_add_or_sub for result).

\param i_result
is the operator index for the result operator.

\param i_arg
is the operator index for the argument to the result operator.

\param op_usage
structure that holds the information for each of the operators.
The output value of op_usage[i_arg] is increased; to be specific,
If sum_result is true and the input value of op_usage[i_arg]
is usage_t(no_usage), its output value is usage_t(csum_usage).
Otherwise, the output value of op_usage[i_arg] is usage_t(yes_usage).

\param cexp_set
This is a vector of sets with one set for each operator. We denote
the i-th set by set[i].

\li
In the special case where cexp_set.n_set() is zero,
cexp_set is not changed.

\li
If cexp_set.n_set() != 0 and op_usage[i_arg] == usage_t(no_usage),
the input value of set[i_arg] must be empty.
In this case the output value if set[i_arg] is equal to set[i_result]
(which may also be empty).

\li
If cexp_set.n_set() != 0 and op_usage[i_arg] != usage_t(no_usage),
the output value of set[i_arg] is the intersection of
its input value and set[i_result].
*/
template <class Base>
inline void op_inc_arg_usage(
	const player<Base>*         play           ,
	bool                        sum_result     ,
	size_t                      i_result       ,
	size_t                      i_arg          ,
	pod_vector<usage_t>&        op_usage       ,
	sparse_list&                cexp_set       )
{	// value of argument input on input to this routine
	enum_usage arg_usage = enum_usage( op_usage[i_arg] );
	//
	// new value for usage
	op_usage[i_arg] = usage_t(yes_usage);
	if( sum_result )
	{	if( arg_usage == no_usage )
		{	OpCode op_a = play->GetOp(i_arg);
			if( op_add_or_sub( op_a ) )
			{	op_usage[i_arg] = usage_t(csum_usage);
			}
		}
	}
	//
	// cexp_set
	if( cexp_set.n_set() == 0 )
		return;
	//
	if( arg_usage == no_usage )
	{	// set[i_arg] = set[i_result]
		cexp_set.assignment(i_arg, i_result, cexp_set);
	}
	else
	{	// set[i_arg] = set[i_arg] intersect set[i_result]
		cexp_set.binary_intersection(i_arg, i_arg, i_result, cexp_set);
	}
	//
	return;
}

/*!
Use reverse activity analysis to get usage information for each operator.

\tparam Base
Base type for the operator; i.e., this operation was recorded
using AD<Base> and computations by this routine are done using type Base.

\tparam Addr
Type used by random iterator for the player.

\param conditional_skip
If conditional_skip this is true, the conditional expression information
cexp_info will be calculated.
This may be time intensive and may not have much benefit in the optimized
recording.

\param compare_op
if this is true, arguments are considered used if they appear in compare
operators. This is a side effect because compare operators have boolean
results (and the result is not in the tape; i.e. NumRes(op) is zero
for these operators. (This is an example of a side effect.)

\param print_for_op
if this is true, arguments are considered used if they appear in
print forward operators; i.e., PriOp.
This is also a side effect; i.e. NumRes(PriOp) is zero.

\param play
This is the operation sequence.

\param random_itr
This is a random iterator for the operation sequence.

\param dep_taddr
is a vector of indices for the dependent variables
(where the reverse activity analysis starts).

\param cexp2op
The input size of this vector must be zero.
Upon retun it has size equal to the number of conditional expressions,
CExpOp operators. The value $icode%cexp2op[%j%]%$$ is the operator
index corresponding to the $th j$$ operator.

\param cexp_set
This is a vector of sets that is empty on input.
If conditional_skip is false, cexp_usage is not modified.
Otherwise, set[i] is a set of elements for the i-th operator.
Suppose that e is an element of set[i], j = e / 2, k = e % 2.
If the comparision for the j-th conditional expression is equal to bool(k),
the i-th operator can be skipped (is not used by any of the results).
Note the the j indexs the CExpOp operators in the operation sequence.

\param vecad_used
The input size of this vector must be zero.
Upon retun it has size equal to the number of VecAD vectors
in the operations sequences; i.e., play->num_vecad_vec_rec().
The VecAD vectors are indexed in the order that thier indices apprear
in the one large play->GetVecInd that holds all the VecAD vectors.

\param op_usage
The input size of this vector must be zero.
Upon return it has size equal to the number of operators
in the operation sequence; i.e., num_op = play->nun_var_rec().
The value op_usage[i]
have been set to the usage for
the i-th operator in the operation sequence.
*/

template <class Addr, class Base>
void get_op_usage(
	bool                                        conditional_skip    ,
	bool                                        compare_op          ,
	bool                                        print_for_op        ,
	const player<Base>*                         play                ,
	const play::const_random_iterator<Addr>&    random_itr          ,
	const pod_vector<size_t>&                   dep_taddr           ,
	pod_vector<addr_t>&                         cexp2op             ,
	sparse_list&                                cexp_set            ,
	pod_vector<bool>&                           vecad_used          ,
	pod_vector<usage_t>&                        op_usage            )
{
	CPPAD_ASSERT_UNKNOWN( cexp_set.n_set()  == 0 );
	CPPAD_ASSERT_UNKNOWN( vecad_used.size() == 0 );
	CPPAD_ASSERT_UNKNOWN( op_usage.size()   == 0 );

	// number of operators in the tape
	const size_t num_op = play->num_op_rec();
	//
	// initialize mapping from variable index to operator index
	CPPAD_ASSERT_UNKNOWN(
		size_t( std::numeric_limits<addr_t>::max() ) >= num_op
	);
	// -----------------------------------------------------------------------
	// information about current operator
	OpCode        op;     // operator
	const addr_t* arg;    // arguments
	size_t        i_op;   // operator index
	size_t        i_var;  // variable index of first result
	// -----------------------------------------------------------------------
	// information about atomic function calls
	size_t user_old=0, user_m=0, user_n=0, user_i=0, user_j=0;
	enum_user_state user_state;
	//
	// work space used by user defined atomic functions
	typedef std::set<size_t> size_set;
	vector<Base>     user_x;       // parameters in x as integers
	vector<size_t>   user_ix;      // variables indices for argument vector
	vector<size_set> user_r_set;   // set sparsity pattern for result
	vector<size_set> user_s_set;   // set sparisty pattern for argument
	vector<bool>     user_r_bool;  // bool sparsity pattern for result
	vector<bool>     user_s_bool;  // bool sparisty pattern for argument
	vectorBool       user_r_pack;  // pack sparsity pattern for result
	vectorBool       user_s_pack;  // pack sparisty pattern for argument
	//
	atomic_base<Base>* user_atom = CPPAD_NULL; // current user atomic function
	bool               user_pack = false;      // sparsity pattern type is pack
	bool               user_bool = false;      // sparsity pattern type is bool
	bool               user_set  = false;      // sparsity pattern type is set
	//
	// parameter information (used by atomic function calls)
	size_t num_par = play->num_par_rec();
	const Base* parameter = CPPAD_NULL;
	if( num_par > 0 )
		parameter = play->GetPar();
	// -----------------------------------------------------------------------
	// vecad information
	size_t num_vecad      = play->num_vecad_vec_rec();
	size_t num_vecad_ind  = play->num_vec_ind_rec();
	//
	vecad_used.resize(num_vecad);
	for(size_t i = 0; i < num_vecad; i++)
		vecad_used[i] = false;
	//
	vector<size_t> arg2vecad(num_vecad_ind);
	for(size_t i = 0; i < num_vecad_ind; i++)
		arg2vecad[i] = num_vecad; // invalid value
	size_t arg_0 = 1; // value of arg[0] for theh first vecad
	for(size_t i = 0; i < num_vecad; i++)
	{
		// mapping from arg[0] value to index for this vecad object.
		arg2vecad[arg_0] = i;
		//
		// length of this vecad object
		size_t length = play->GetVecInd(arg_0 - 1);
		//
		// set to proper index in GetVecInd for next VecAD arg[0] value
		arg_0        += length + 1;
	}
	CPPAD_ASSERT_UNKNOWN( arg_0 == num_vecad_ind + 1 );
	// -----------------------------------------------------------------------
	// conditional expression information
	//
	size_t num_cexp_op = 0;
	if( conditional_skip )
	{	for(i_op = 0; i_op < num_op; ++i_op)
		{	if( random_itr.get_op(i_op) == CExpOp )
			{	// count the number of conditional expressions.
				++num_cexp_op;
			}
		}
	}
	//
	cexp2op.resize( num_cexp_op );
	//
	// number of sets
	size_t num_set = 0;
	if( conditional_skip && num_cexp_op > 0)
		num_set = num_op;
	//
	// conditional expression index   = element / 2
	// conditional expression compare = bool ( element % 2)
	size_t end_set = 2 * num_cexp_op;
	//
	if( num_set > 0 )
		cexp_set.resize(num_set, end_set);
	// -----------------------------------------------------------------------
	// initilaize operator usage for reverse dependency analysis.
	op_usage.resize( num_op );
	for(i_op = 0; i_op < num_op; ++i_op)
		op_usage[i_op] = usage_t(no_usage);
	for(size_t i = 0; i < dep_taddr.size(); i++)
	{	i_op           = random_itr.var2op(dep_taddr[i]);
		op_usage[i_op] = usage_t(yes_usage);    // dependent variables
	}
	// ----------------------------------------------------------------------
	// Reverse pass to compute usage and cexp_set for each operator
	// ----------------------------------------------------------------------
	//
	// Initialize reverse pass
	size_t last_user_i_op = 0;
	size_t cexp_index     = num_cexp_op;
	user_state            = end_user;
	i_op = num_op;
	while(i_op != 0 )
	{	--i_op;
		//
		// this operator information
		random_itr.op_info(i_op, op, arg, i_var);
		//
		// Is the result of this operation used.
		// (This only makes sense when NumRes(op) > 0.)
		usage_t use_result = op_usage[i_op];
		//
		bool sum_op = false;
		switch( op )
		{
			// =============================================================
			// normal operators
			// =============================================================

			// Only one variable with index arg[0]
			case SubvpOp:
			sum_op = true;
			//
			case AbsOp:
			case AcosOp:
			case AcoshOp:
			case AsinOp:
			case AsinhOp:
			case AtanOp:
			case AtanhOp:
			case CosOp:
			case CoshOp:
			case DivvpOp:
			case ErfOp:
			case ExpOp:
			case Expm1Op:
			case LogOp:
			case Log1pOp:
			case PowvpOp:
			case SignOp:
			case SinOp:
			case SinhOp:
			case SqrtOp:
			case TanOp:
			case TanhOp:
			case ZmulvpOp:
			CPPAD_ASSERT_UNKNOWN( NumRes(op) > 0 );
			if( use_result != usage_t(no_usage) )
			{	size_t j_op = random_itr.var2op(size_t(arg[0]));
				op_inc_arg_usage(
					play, sum_op, i_op, j_op, op_usage, cexp_set
				);
			}
			break; // --------------------------------------------

			// Only one variable with index arg[1]
			case AddpvOp:
			case SubpvOp:
			sum_op = true;
			//
			case DisOp:
			case DivpvOp:
			case MulpvOp:
			case PowpvOp:
			case ZmulpvOp:
			CPPAD_ASSERT_UNKNOWN( NumRes(op) > 0 );
			if( use_result != usage_t(no_usage) )
			{	size_t j_op = random_itr.var2op(size_t(arg[1]));
				op_inc_arg_usage(
					play, sum_op, i_op, j_op, op_usage, cexp_set
				);
			}
			break; // --------------------------------------------

			// arg[0] and arg[1] are the only variables
			case AddvvOp:
			case SubvvOp:
			sum_op = true;
			//
			case DivvvOp:
			case MulvvOp:
			case PowvvOp:
			case ZmulvvOp:
			CPPAD_ASSERT_UNKNOWN( NumRes(op) > 0 );
			if( use_result != usage_t(no_usage) )
			{	for(size_t i = 0; i < 2; i++)
				{	size_t j_op = random_itr.var2op(size_t(arg[i]));
					op_inc_arg_usage(
						play, sum_op, i_op, j_op, op_usage, cexp_set
					);
				}
			}
			break; // --------------------------------------------

			// Conditional expression operators
			// arg[2], arg[3], arg[4], arg[5] are parameters or variables
			case CExpOp:
			CPPAD_ASSERT_UNKNOWN( NumRes(op) > 0 );
			if( conditional_skip )
			{	--cexp_index;
				cexp2op[ cexp_index ] = addr_t(i_op);
			}
			if( use_result != usage_t(no_usage) )
			{	CPPAD_ASSERT_UNKNOWN( NumArg(CExpOp) == 6 );
				// propgate from result to left argument
				if( arg[1] & 1 )
				{	size_t j_op = random_itr.var2op(size_t(arg[2]));
					op_inc_arg_usage(
						play, sum_op, i_op, j_op, op_usage, cexp_set
					);
				}
				// propgate from result to right argument
				if( arg[1] & 2 )
				{	size_t j_op = random_itr.var2op(size_t(arg[3]));
					op_inc_arg_usage(
							play, sum_op, i_op, j_op, op_usage, cexp_set
					);
				}
				// are if_true and if_false cases the same variable
				bool same_variable = (arg[1] & 4) != 0;
				same_variable     &= (arg[1] & 8) != 0;
				same_variable     &= arg[4] == arg[5];
				//
				// if_true
				if( arg[1] & 4 )
				{	size_t j_op = random_itr.var2op(size_t(arg[4]));
					bool can_skip = conditional_skip & (! same_variable);
					can_skip     &= op_usage[j_op] == usage_t(no_usage);
					op_inc_arg_usage(
						play, sum_op, i_op, j_op, op_usage, cexp_set
					);
					if( can_skip )
					{	// j_op corresponds to the value used when the
						// comparison result is true. It can be skipped when
						// the comparison is false (0).
						size_t element = 2 * cexp_index + 0;
						cexp_set.add_element(j_op, element);
						//
						op_usage[j_op] = usage_t(yes_usage);
					}
				}
				//
				// if_false
				if( arg[1] & 8 )
				{	size_t j_op = random_itr.var2op(size_t(arg[5]));
					bool can_skip = conditional_skip & (! same_variable);
					can_skip     &= op_usage[j_op] == usage_t(no_usage);
					op_inc_arg_usage(
						play, sum_op, i_op, j_op, op_usage, cexp_set
					);
					if( can_skip )
					{	// j_op corresponds to the value used when the
						// comparison result is false. It can be skipped when
						// the comparison is true (0).
						size_t element = 2 * cexp_index + 1;
						cexp_set.add_element(j_op, element);
						//
						op_usage[j_op] = usage_t(yes_usage);
					}
				}
			}
			break;  // --------------------------------------------

			// Operations that are never used
			// (new CSkip options are generated if conditional_skip is true)
			case CSkipOp:
			case ParOp:
			break;

			// Operators that are always used
			case InvOp:
			case BeginOp:
			case EndOp:
			op_usage[i_op] = usage_t(yes_usage);
			break;  // -----------------------------------------------

			// The print forward operator
			case PriOp:
			CPPAD_ASSERT_NARG_NRES(op, 5, 0);
			if( print_for_op )
			{	op_usage[i_op] = usage_t(yes_usage);
				if( arg[0] & 1 )
				{	// arg[1] is a variable
					size_t j_op = random_itr.var2op(size_t(arg[1]));
					op_inc_arg_usage(
						play, sum_op, i_op, j_op, op_usage, cexp_set
					);
				}
				if( arg[0] & 2 )
				{	// arg[3] is a variable
					size_t j_op = random_itr.var2op(size_t(arg[3]));
					op_inc_arg_usage(
						play, sum_op, i_op, j_op, op_usage, cexp_set
					);
				}
			}
			break; // -----------------------------------------------------

			// =============================================================
			// Comparison operators
			// =============================================================

			// Compare operators where arg[1] is only variable
			case LepvOp:
			case LtpvOp:
			case EqpvOp:
			case NepvOp:
			CPPAD_ASSERT_UNKNOWN( NumRes(op) == 0 );
			if( compare_op )
			{	op_usage[i_op] = usage_t(yes_usage);
				//
				size_t j_op = random_itr.var2op(size_t(arg[1]));
				op_inc_arg_usage(
					play, sum_op, i_op, j_op, op_usage, cexp_set
				);
			}
			break; // ----------------------------------------------

			// Compare operators where arg[0] is only variable
			case LevpOp:
			case LtvpOp:
			CPPAD_ASSERT_UNKNOWN( NumRes(op) == 0 );
			if( compare_op )
			{	op_usage[i_op] = usage_t(yes_usage);
				//
				size_t j_op = random_itr.var2op(size_t(arg[0]));
				op_inc_arg_usage(
					play, sum_op, i_op, j_op, op_usage, cexp_set
				);
			}
			break; // ----------------------------------------------

			// Compare operators where arg[0] and arg[1] are variables
			case LevvOp:
			case LtvvOp:
			case EqvvOp:
			case NevvOp:
			CPPAD_ASSERT_UNKNOWN( NumRes(op) == 0 );
			if( compare_op )
			{	op_usage[i_op] = usage_t(yes_usage);
				//
				for(size_t i = 0; i < 2; i++)
				{	size_t j_op = random_itr.var2op(size_t(arg[i]));
					op_inc_arg_usage(
						play, sum_op, i_op, j_op, op_usage, cexp_set
					);
				}
			}
			break; // ----------------------------------------------

			// =============================================================
			// VecAD operators
			// =============================================================

			// load operator using a parameter index
			case LdpOp:
			CPPAD_ASSERT_UNKNOWN( NumRes(op) > 0 );
			if( use_result != usage_t(no_usage) )
			{	size_t i_vec = arg2vecad[ arg[0] ];
				vecad_used[i_vec] = true;
			}
			break; // --------------------------------------------

			// load operator using a variable index
			case LdvOp:
			CPPAD_ASSERT_UNKNOWN( NumRes(op) > 0 );
			if( use_result != usage_t(no_usage) )
			{	size_t i_vec = arg2vecad[ arg[0] ];
				vecad_used[i_vec] = true;
				//
				size_t j_op = random_itr.var2op(size_t(arg[1]));
				op_usage[j_op] = usage_t(yes_usage);
			}
			break; // --------------------------------------------

			// Store a variable using a parameter index
			case StpvOp:
			CPPAD_ASSERT_UNKNOWN( NumRes(op) == 0 );
			if( vecad_used[ arg2vecad[ arg[0] ] ] )
			{	op_usage[i_op] = usage_t(yes_usage);
				//
				size_t j_op = random_itr.var2op(size_t(arg[2]));
				op_usage[j_op] = usage_t(yes_usage);
			}
			break; // --------------------------------------------

			// Store a variable using a variable index
			case StvvOp:
			CPPAD_ASSERT_UNKNOWN( NumRes(op) == 0 );
			if( vecad_used[ arg2vecad[ arg[0] ] ] )
			{	op_usage[i_op] = usage_t(yes_usage);
				//
				size_t j_op = random_itr.var2op(size_t(arg[1]));
				op_usage[j_op] = usage_t(yes_usage);
				size_t k_op = random_itr.var2op(size_t(arg[2]));
				op_usage[k_op] = usage_t(yes_usage);
			}
			break; // -----------------------------------------------------

			// =============================================================
			// cumulative summation operator
			// ============================================================
			case CSumOp:
			CPPAD_ASSERT_UNKNOWN( NumRes(op) == 1 );
			{
				for(size_t i = 5; i < size_t(arg[2]); i++)
				{	size_t j_op = random_itr.var2op(size_t(arg[i]));
					op_inc_arg_usage(
						play, sum_op, i_op, j_op, op_usage, cexp_set
					);
				}
			}
			// =============================================================
			// user defined atomic operators
			// ============================================================

			case UserOp:
			// start or end atomic operation sequence
			if( user_state == end_user )
			{	// revese_user using random_itr instead of play
				size_t user_index = size_t(arg[0]);
				user_old          = size_t(arg[1]);
				user_n            = size_t(arg[2]);
				user_m            = size_t(arg[3]);
				user_j            = user_n;
				user_i            = user_m;
				user_state        = ret_user;
				user_atom  = atomic_base<Base>::class_object(user_index);
				// -------------------------------------------------------
				last_user_i_op = i_op;
				CPPAD_ASSERT_UNKNOWN( i_op > user_n + user_m + 1 );
				CPPAD_ASSERT_UNKNOWN(
					op_usage[last_user_i_op] == usage_t(no_usage)
				);
# ifndef NDEBUG
				if( cexp_set.n_set() > 0 )
				{	sparse_list_const_iterator itr(cexp_set, last_user_i_op);
					CPPAD_ASSERT_UNKNOWN( *itr == cexp_set.end() );
				}
# endif
				//
				user_x.resize(  user_n );
				user_ix.resize( user_n );
				//
				user_pack  = user_atom->sparsity() ==
							atomic_base<Base>::pack_sparsity_enum;
				user_bool  = user_atom->sparsity() ==
							atomic_base<Base>::bool_sparsity_enum;
				user_set   = user_atom->sparsity() ==
							atomic_base<Base>::set_sparsity_enum;
				CPPAD_ASSERT_UNKNOWN( user_pack || user_bool || user_set );
				//
				// Note that q is one for this call the sparsity calculation
				if( user_pack )
				{	user_r_pack.resize( user_m );
					user_s_pack.resize( user_n );
					for(size_t i = 0; i < user_m; i++)
						user_r_pack[ i ] = false;
				}
				if( user_bool )
				{	user_r_bool.resize( user_m );
					user_s_bool.resize( user_n );
					for(size_t i = 0; i < user_m; i++)
						user_r_bool[ i ] = false;
				}
				if( user_set )
				{	user_s_set.resize(user_n);
					user_r_set.resize(user_m);
					for(size_t i = 0; i < user_m; i++)
						user_r_set[i].clear();
				}
			}
			else
			{	// reverse_user using random_itr instead of play
				CPPAD_ASSERT_UNKNOWN( user_state == start_user );
				CPPAD_ASSERT_UNKNOWN( user_n == size_t(arg[2]) );
				CPPAD_ASSERT_UNKNOWN( user_m == size_t(arg[3]) );
				CPPAD_ASSERT_UNKNOWN( user_j == 0 );
				CPPAD_ASSERT_UNKNOWN( user_i == 0 );
				user_state = end_user;
				// -------------------------------------------------------
				CPPAD_ASSERT_UNKNOWN(
					i_op + user_n + user_m + 1 == last_user_i_op
				);
				// call users function for this operation
				user_atom->set_old(user_old);
				bool user_ok  = false;
				size_t user_q = 1; // as if sum of dependent variables
				if( user_pack )
				{	user_ok = user_atom->rev_sparse_jac(
						user_q, user_r_pack, user_s_pack, user_x
					);
					if( ! user_ok ) user_ok = user_atom->rev_sparse_jac(
						user_q, user_r_pack, user_s_pack
					);
				}
				if( user_bool )
				{	user_ok = user_atom->rev_sparse_jac(
						user_q, user_r_bool, user_s_bool, user_x
					);
					if( ! user_ok ) user_ok = user_atom->rev_sparse_jac(
						user_q, user_r_bool, user_s_bool
					);
				}
				if( user_set )
				{	user_ok = user_atom->rev_sparse_jac(
						user_q, user_r_set, user_s_set, user_x
					);
					if( ! user_ok ) user_ok = user_atom->rev_sparse_jac(
						user_q, user_r_set, user_s_set
					);
				}
				if( ! user_ok )
				{	std::string s =
						"Optimizing an ADFun object"
						" that contains the atomic function\n\t";
					s += user_atom->afun_name();
					s += "\nCurrent atomic_sparsity is set to ";
					//
					if( user_set )
						s += "set_sparsity_enum.\n";
					if( user_bool )
						s += "bool_sparsity_enum.\n";
					if( user_pack )
						s += "pack_sparsity_enum.\n";
					//
					s += "This version of rev_sparse_jac returned false";
					CPPAD_ASSERT_KNOWN(false, s.c_str() );
				}

				if( op_usage[last_user_i_op] != usage_t(no_usage) )
				for(size_t j = 0; j < user_n; j++)
				if( user_ix[j] > 0 )
				{	// This user argument is a variable
					bool use_arg_j = false;
					if( user_set )
					{	if( ! user_s_set[j].empty() )
							use_arg_j = true;
					}
					if( user_bool )
					{	if( user_s_bool[j] )
							use_arg_j = true;
					}
					if( user_pack )
					{	if( user_s_pack[j] )
							use_arg_j = true;
					}
					if( use_arg_j )
					{	size_t j_op = random_itr.var2op(user_ix[j]);
						op_inc_arg_usage(play,
							sum_op, last_user_i_op, j_op, op_usage, cexp_set
						);
					}
				}
				// copy set infomation from last to first
				if( cexp_set.n_set() > 0 )
					cexp_set.assignment(i_op, last_user_i_op, cexp_set);
				// copy user information from last to all the user operators
				// for this call
				for(size_t j = 0; j < user_n + user_m + 1; ++j)
					op_usage[i_op + j] = op_usage[last_user_i_op];
			}
			break; // -------------------------------------------------------

			case UsrapOp:
			// parameter argument in an atomic operation sequence
			CPPAD_ASSERT_UNKNOWN( size_t(arg[0]) < num_par );
			//
			// reverse_user using random_itr instead of play
			CPPAD_ASSERT_NARG_NRES(op, 1, 0);
			CPPAD_ASSERT_UNKNOWN( 0 < user_j && user_j < user_n );
			--user_j;
			if( user_j == 0 )
				user_state = start_user;
			// -------------------------------------------------------------
			user_ix[user_j] = 0;
			//
			// parameter arguments
			user_x[user_j] = parameter[arg[0]];
			//
			break;

			case UsravOp:
			// variable argument in an atomic operation sequence
			CPPAD_ASSERT_UNKNOWN( 0 < arg[0] );
			//
			// reverse_user using random_itr instead of play
			CPPAD_ASSERT_NARG_NRES(op, 1, 0);
			CPPAD_ASSERT_UNKNOWN( 0 < user_j && user_j <= user_n );
			--user_j;
			if( user_j == 0 )
				user_state = start_user;
			// -------------------------------------------------------------
			user_ix[user_j] = size_t(arg[0]);
			//
			// variable arguments as parameters
			user_x[user_j] = CppAD::numeric_limits<Base>::quiet_NaN();
			//
			break;

			case UsrrvOp:
			// variable result in an atomic operation sequence
			//
			// reverse_user using random_itr instead of play
			CPPAD_ASSERT_NARG_NRES(op, 0, 1);
			CPPAD_ASSERT_UNKNOWN( 0 < user_i && user_i <= user_m );
			--user_i;
			if( user_i == 0 )
				user_state = arg_user;
			// -------------------------------------------------------------
			if( use_result )
			{	if( user_set )
					user_r_set[user_i].insert(0);
				if( user_bool )
					user_r_bool[user_i] = true;
				if( user_pack )
					user_r_pack[user_i] = true;
				//
				op_inc_arg_usage(
					play, sum_op, i_op, last_user_i_op, op_usage, cexp_set
				);
			}
			break; // --------------------------------------------------------

			case UsrrpOp:
			CPPAD_ASSERT_UNKNOWN( size_t(arg[0]) < num_par );
			//
			// reverse_user using random_itr instead of play
			CPPAD_ASSERT_NARG_NRES(op, 0, 1);
			CPPAD_ASSERT_UNKNOWN( 0 < user_i && user_i < user_m );
			--user_i;
			if( user_i == 0 )
				user_state = arg_user;
			break;
			// ============================================================

			// all cases should be handled above
			default:
			CPPAD_ASSERT_UNKNOWN(0);
		}
	}
	return;
}

} } } // END_CPPAD_LOCAL_OPTIMIZE_NAMESPACE

# endif

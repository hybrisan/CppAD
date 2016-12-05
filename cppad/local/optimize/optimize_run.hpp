// $Id$

# ifndef CPPAD_LOCAL_OPTIMIZE_OPTIMIZE_RUN_HPP
# define CPPAD_LOCAL_OPTIMIZE_OPTIMIZE_RUN_HPP

/* --------------------------------------------------------------------------
CppAD: C++ Algorithmic Differentiation: Copyright (C) 2003-16 Bradley M. Bell

CppAD is distributed under multiple licenses. This distribution is under
the terms of the
                    Eclipse Public License Version 1.0.

A copy of this license is included in the COPYING file of this distribution.
Please visit http://www.coin-or.org/CppAD/ for information on other licenses.
-------------------------------------------------------------------------- */

# include <stack>
# include <iterator>
# include <cppad/local/optimize/op_info.hpp>
# include <cppad/local/optimize/old2new.hpp>
# include <cppad/local/optimize/connect_type.hpp>
# include <cppad/local/optimize/fast_empty_set.hpp>
# include <cppad/local/optimize/size_pair.hpp>
# include <cppad/local/optimize/csum_variable.hpp>
# include <cppad/local/optimize/csum_stacks.hpp>
# include <cppad/local/optimize/cskip_info.hpp>
# include <cppad/local/optimize/unary_match.hpp>
# include <cppad/local/optimize/binary_match.hpp>
# include <cppad/local/optimize/record_pv.hpp>
# include <cppad/local/optimize/record_vp.hpp>
# include <cppad/local/optimize/record_vv.hpp>
# include <cppad/local/optimize/record_csum.hpp>

/*!
\file optimize_run.hpp
Convert a player object to an optimized recorder object
*/
// BEGIN_CPPAD_LOCAL_OPTIMIZE_NAMESPACE
namespace CppAD { namespace local { namespace optimize  {
/*!
Convert a player object to an optimized recorder object

\tparam Base
base type for the operator; i.e., this operation was recorded
using AD< \a Base > and computations by this routine are done using type
\a Base.

\param options
\li
If the sub-string "no_conditional_skip" appears,
no conditional skip operations will be generated.
\li
If the sub-string "no_compare_op" appears,
then comparison operators will be removed.
This means that the compare_change function will no-longer
be meaningful for the resulting recording.

\param n
is the number of independent variables on the tape.

\param dep_taddr
On input this vector contains the indices for each of the dependent
variable values in the operation sequence corresponding to \a play.
Upon return it contains the indices for the same variables but in
the operation sequence corresponding to \a rec.

\param play
This is the operation sequence that we are optimizing.
It is essentially const, except for play back state which
changes while it plays back the operation seqeunce.

\param rec
The input contents of this recording does not matter.
Upon return, it contains an optimized verison of the
operation sequence corresponding to \a play.
*/

template <class Base>
void optimize_run(
	const std::string&           options   ,
	size_t                       n         ,
	CppAD::vector<size_t>&       dep_taddr ,
	player<Base>*                play      ,
	recorder<Base>*              rec       )
{
	bool conditional_skip =
		options.find("no_conditional_skip", 0) == std::string::npos;

	bool compare_op =
		options.find("no_compare_op", 0) == std::string::npos;

	// number of operators in the player
	const size_t num_op = play->num_op_rec();

	// number of variables in the player
	const size_t num_var = play->num_var_rec();

	// number of  VecAD indices
	size_t num_vecad_ind   = play->num_vec_ind_rec();

	// number of VecAD vectors
	size_t num_vecad_vec   = play->num_vecad_vec_rec();

	// operator information
	vector<size_t>          var2op, cexp2op;
	vector<bool>            vecad_used;
	vector<struct_op_info>  op_info;
	get_op_info(
		conditional_skip,
		compare_op,
		play,
		dep_taddr,
		var2op,
		cexp2op,
		vecad_used,
		op_info
	);
	// number of conditonal expressions
	size_t num_cexp_op = cexp2op.size();

	// nan with type Base
	Base base_nan = Base( std::numeric_limits<double>::quiet_NaN() );

	// information for current operator
	size_t        i_op;   // index
	OpCode        op;     // operator
	const addr_t* arg;    // arguments
	size_t        i_var;  // variable index of primary (last) result

	// information defined by forward_user
	size_t user_old=0, user_m=0, user_n=0, user_i=0, user_j=0;
	enum_user_state user_state;
	//
	/// information for each CSkip operation
	CppAD::vector<struct_cskip_info>   cskip_info(num_cexp_op);
	for(size_t i = 0; i < num_cexp_op; i++)
	{	i_op            = cexp2op[i];
		arg             = op_info[i_op].arg;
		CPPAD_ASSERT_UNKNOWN( op_info[i_op].op == CExpOp );
		//
		struct_cskip_info info;
		info.i_op       = i_op;
		info.cop        = CompareOp( arg[0] );
		info.flag       = arg[1];
		info.left       = arg[2];
		info.right      = arg[3];
		info.i_arg      = 0; // case where no CSkipOp for this CExpOp
		//
		// max_left_right
		size_t index    = 0;
		if( arg[1] & 1 )
			index = std::max(index, info.left);
		if( arg[1] & 2 )
			index = std::max(index, info.right);
		CPPAD_ASSERT_UNKNOWN( index > 0 );
		info.max_left_right = index;
		//
		cskip_info[i] = info;
	};


	// Determine which operators can be conditionally skipped
	for(size_t i = 0; i < num_op; i++)
	{	if( ! op_info[i].cexp_set.empty() )
		{
			fast_empty_set<cexp_compare>::const_iterator itr =
				op_info[i].cexp_set.begin();
			while( itr != op_info[i].cexp_set.end() )
			{	size_t j = itr->index();
				if( itr->compare() == false )
					cskip_info[j].skip_old_op_false.push_back(i);
				else cskip_info[j].skip_old_op_true.push_back(i);
				itr++;
			}
		}
	}
	// -------------------------------------------------------------
	// Sort the conditional skip information by the maximum of the
	// index for the left and right comparision operands
	CppAD::vector<size_t> cskip_info_order( cskip_info.size() );
	{	CppAD::vector<size_t> keys( cskip_info.size() );
		for(size_t i = 0; i < cskip_info.size(); i++)
			keys[i] = std::max( cskip_info[i].left, cskip_info[i].right );
		CppAD::index_sort(keys, cskip_info_order);
	}
	// index in sorted order
	size_t cskip_order_next = 0;
	// index in order during reverse sweep
	size_t cskip_info_index = cskip_info.size();


	// Initilaize table mapping hash code to variable index in tape
	// as pointing to the BeginOp at the beginning of the tape
	CppAD::vector<size_t>  hash_table_var(CPPAD_HASH_TABLE_SIZE);
	for(size_t i = 0; i < CPPAD_HASH_TABLE_SIZE; i++)
		hash_table_var[i] = 0;
	CPPAD_ASSERT_UNKNOWN( op_info[0].op == BeginOp );

	// Erase all information in the old recording
	rec->free();

	// initialize mapping from old VecAD index to new VecAD index
	CppAD::vector<size_t> new_vecad_ind(num_vecad_ind);
	for(size_t i = 0; i < num_vecad_ind; i++)
		new_vecad_ind[i] = num_vecad_ind; // invalid index
	{
		size_t j = 0;     // index into the old set of indices
		for(size_t i = 0; i < num_vecad_vec; i++)
		{	// length of this VecAD
			size_t length = play->GetVecInd(j);
			if( vecad_used[i] )
			{	// Put this VecAD vector in new recording
				CPPAD_ASSERT_UNKNOWN(length < num_vecad_ind);
				new_vecad_ind[j] = rec->PutVecInd(length);
				for(size_t k = 1; k <= length; k++) new_vecad_ind[j+k] =
					rec->PutVecInd(
						rec->PutPar(
							play->GetPar(
								play->GetVecInd(j+k)
				) ) );
			}
			// start of next VecAD
			j       += length + 1;
		}
		CPPAD_ASSERT_UNKNOWN( j == num_vecad_ind );
	}
	//
	// Mapping from old operator index to new operator information
	// (zero is invalid except for old2new[0].new_op and old2new[0].i_var)
	vector<struct_old2new> old2new(num_op);
	for(size_t i = 0; i < num_op; i++)
	{	old2new[i].new_op  = 0;
		old2new[i].new_var = 0;
	}


	// temporary buffer for new argument values
	addr_t new_arg[6];

	// temporary work space used by record_csum
	// (decalared here to avoid realloaction of memory)
	struct_csum_stacks csum_work;

	// tempory used to hold a size_pair
	struct_size_pair size_pair;

	// start playing the operations in the forward direction
	play->forward_start(op, arg, i_op, i_var);
	CPPAD_ASSERT_UNKNOWN( op == BeginOp );
	CPPAD_ASSERT_NARG_NRES(BeginOp, 1, 1);
	CPPAD_ASSERT_UNKNOWN( size_t(arg[0]) == 0 );
	//
	// Put BeginOp at beginning of recording
	old2new[i_op].new_op  = rec->num_op_rec();
	old2new[i_op].new_var = rec->PutOp(BeginOp);
	rec->PutArg(arg[0]);
	//
	size_t first_user_i_op = 0;
	user_state = start_user;
	while( op != EndOp )
	{	addr_t mask; // temporary used in some switch cases
		//
		// this op
		play->forward_next(op, arg, i_op, i_var);

		// determine if we should insert a conditional skip here
		bool skip = cskip_order_next < cskip_info.size();
		skip     &= op != BeginOp;
		skip     &= op != InvOp;
		skip     &= user_state == start_user;
		if( skip )
		{	size_t j = cskip_info_order[cskip_order_next];
			if( NumRes(op) > 0 )
				skip &= cskip_info[j].max_left_right < i_var;
			else
				skip &= cskip_info[j].max_left_right <= i_var;
		}
		if( skip )
		{	size_t j = cskip_info_order[cskip_order_next];
			cskip_order_next++;
			struct_cskip_info info = cskip_info[j];
			size_t n_true  = info.skip_old_op_true.size();
			size_t n_false = info.skip_old_op_false.size();
			skip &= n_true > 0 || n_false > 0;
			if( skip )
			{	CPPAD_ASSERT_UNKNOWN( NumRes(CSkipOp) == 0 );
				size_t n_arg   = 7 + n_true + n_false;
				// reserve space for the arguments to this operator but
				// delay setting them until we have all the new addresses
				cskip_info[j].i_arg = rec->ReserveArg(n_arg);
				CPPAD_ASSERT_UNKNOWN( cskip_info[j].i_arg > 0 );
				// There is no corresponding old operator in this case
				rec->PutOp(CSkipOp);
			}
		}
		//
		size_t usage_i_op = i_op;
		switch(op)
		{	case CSkipOp:
			play->forward_cskip(op, arg, i_op, i_var);
			break;

			case CSumOp:
			play->forward_csum(op, arg, i_op, i_var);
			break;

			case UserOp:
			// index of the first operator in the atomic user function call
			if( user_state == start_user )
				first_user_i_op = i_op;
			//
			case UsrapOp:
			case UsravOp:
			case UsrrpOp:
			case UsrrvOp:
			play->forward_user(op, user_state,
				user_old, user_m, user_n, user_i, user_j
			);
			// All the operators for a user atomic function call act as a block
			usage_i_op = first_user_i_op;
			break;

			default:
			break;
		}
		//
		unsigned short code         = 0;
		bool           replace_hash = false;
		addr_t         match_var;
		old2new[i_op].match = false;
		//
		if( op_info[usage_i_op].usage > 0 ) switch( op )
		{
			// Unary operator where operand is arg[0]
			case AbsOp:
			case AcosOp:
			case AcoshOp:
			case AsinOp:
			case AsinhOp:
			case AtanOp:
			case AtanhOp:
			case CosOp:
			case CoshOp:
			case ErfOp:
			case ExpOp:
			case Expm1Op:
			case LogOp:
			case Log1pOp:
			case SignOp:
			case SinOp:
			case SinhOp:
			case SqrtOp:
			case TanOp:
			case TanhOp:
			match_var = unary_match(
				var2op              ,
				op_info             ,
				old2new             ,
				i_var               ,
				play->num_par_rec() ,
				play->GetPar()      ,
				hash_table_var      ,
				code
			);
			if( match_var > 0 )
			{	size_t j_op = var2op[match_var];
				//
				old2new[i_op].match   = true;
				old2new[j_op].match   = true;
				old2new[i_op].new_var = old2new[j_op].new_var;
			}
			else
			{
				replace_hash = true;
				new_arg[0]   = old2new[ var2op[arg[0]] ].new_var;
				rec->PutArg( new_arg[0] );
				//
				old2new[i_op].new_op  = rec->num_op_rec();
				old2new[i_op].new_var = rec->PutOp(op);
				CPPAD_ASSERT_UNKNOWN(new_arg[0] < old2new[var2op[i_var]].new_var);
				if( op == ErfOp )
				{	// Error function is a special case
					// second argument is always the parameter 0
					// third argument is always the parameter 2 / sqrt(pi)
					CPPAD_ASSERT_UNKNOWN( NumArg(ErfOp) == 3 );
					rec->PutArg( rec->PutPar( Base(0) ) );
					rec->PutArg( rec->PutPar(
						Base( 1.0 / std::sqrt( std::atan(1.0) ) )
					) );
				}
			}
			break;
			// ---------------------------------------------------
			// Binary operators where
			// left is a variable and right is a parameter
			case SubvpOp:
			// check if this is the top of a csum connection
			if( op_info[i_op].csum_connected )
				break;
			if( op_info[ var2op[arg[0]] ].csum_connected )
			{
				// convert to a sequence of summation operators
				size_pair = record_csum(
					var2op              ,
					op_info             ,
					old2new             ,
					i_var               ,
					play->num_par_rec() ,
					play->GetPar()      ,
					rec                 ,
					csum_work
				);
				old2new[i_op].new_op  = size_pair.i_op;
				old2new[i_op].new_var = size_pair.i_var;
				// abort rest of this case
				break;
			}
			case DivvpOp:
			case PowvpOp:
			case ZmulvpOp:
			match_var = binary_match(
				var2op              ,
				op_info             ,
				old2new             ,
				i_var               ,
				play->num_par_rec() ,
				play->GetPar()      ,
				hash_table_var      ,
				code
			);
			if( match_var > 0 )
			{	size_t j_op = var2op[match_var];
				//
				old2new[i_op].match   = true;
				old2new[j_op].match   = true;
				old2new[i_op].new_var = old2new[j_op].new_var;
			}
			else
			{	size_pair = record_vp(
					var2op              ,
					op_info             ,
					old2new             ,
					i_var               ,
					play->num_par_rec() ,
					play->GetPar()      ,
					rec                 ,
					op                  ,
					arg
				);
				old2new[i_op].new_op  = size_pair.i_op;
				old2new[i_op].new_var = size_pair.i_var;
				replace_hash = true;
			}
			break;
			// ---------------------------------------------------
			// Binary operators where
			// left is an index and right is a variable
			case DisOp:
			match_var = binary_match(
				var2op              ,
				op_info             ,
				old2new             ,
				i_var               ,
				play->num_par_rec() ,
				play->GetPar()      ,
				hash_table_var      ,
				code
			);
			if( match_var > 0 )
			{	size_t j_op = var2op[match_var];
				//
				old2new[i_op].match   = true;
				old2new[j_op].match   = true;
				old2new[i_op].new_var = old2new[j_op].new_var;
			}
			else
			{	new_arg[0] = arg[0];
				new_arg[1] = old2new[ var2op[arg[1]] ].new_var;
				rec->PutArg( new_arg[0], new_arg[1] );
				//
				old2new[i_op].new_op  = rec->num_op_rec();
				old2new[i_op].new_var = rec->PutOp(op);
				CPPAD_ASSERT_UNKNOWN(
					new_arg[1] < old2new[var2op[i_var]].new_var
				);
				replace_hash = true;
			}
			break;

			// ---------------------------------------------------
			// Binary operators where
			// left is a parameter and right is a variable
			case SubpvOp:
			case AddpvOp:
			// check if this is the top of a csum connection
			if( op_info[i_op].csum_connected )
				break;
			if( op_info[ var2op[arg[1]] ].csum_connected )
			{
				// convert to a sequence of summation operators
				size_pair = record_csum(
					var2op              ,
					op_info             ,
					old2new             ,
					i_var               ,
					play->num_par_rec() ,
					play->GetPar()      ,
					rec                 ,
					csum_work
				);
				old2new[i_op].new_op  = size_pair.i_op;
				old2new[i_op].new_var = size_pair.i_var;
				// abort rest of this case
				break;
			}
			case DivpvOp:
			case MulpvOp:
			case PowpvOp:
			case ZmulpvOp:
			match_var = binary_match(
				var2op              ,
				op_info             ,
				old2new             ,
				i_var               ,
				play->num_par_rec() ,
				play->GetPar()      ,
				hash_table_var      ,
				code
			);
			if( match_var > 0 )
			{	size_t j_op = var2op[match_var];
				//
				old2new[i_op].match   = true;
				old2new[j_op].match   = true;
				old2new[i_op].new_var = old2new[j_op].new_var;
			}
			else
			{	size_pair = record_pv(
					var2op              ,
					op_info             ,
					old2new             ,
					i_var               ,
					play->num_par_rec() ,
					play->GetPar()      ,
					rec                 ,
					op                  ,
					arg
				);
				old2new[i_op].new_op  = size_pair.i_op;
				old2new[i_op].new_var = size_pair.i_var;
				replace_hash = true;
			}
			break;
			// ---------------------------------------------------
			// Binary operator where both operands are variables
			case AddvvOp:
			case SubvvOp:
			// check if this is the top of a csum connection
			if( op_info[i_op].csum_connected )
				break;
			if(
				op_info[ var2op[arg[0]] ].csum_connected |
				op_info[ var2op[arg[1]] ].csum_connected
			)
			{
				// convert to a sequence of summation operators
				size_pair = record_csum(
					var2op              ,
					op_info             ,
					old2new             ,
					i_var               ,
					play->num_par_rec() ,
					play->GetPar()      ,
					rec                 ,
					csum_work
				);
				old2new[i_op].new_op  = size_pair.i_op;
				old2new[i_op].new_var = size_pair.i_var;
				// abort rest of this case
				break;
			}
			case DivvvOp:
			case MulvvOp:
			case PowvvOp:
			case ZmulvvOp:
			match_var = binary_match(
				var2op              ,
				op_info             ,
				old2new             ,
				i_var               ,
				play->num_par_rec() ,
				play->GetPar()      ,
				hash_table_var      ,
				code
			);
			if( match_var > 0 )
			{	size_t j_op = var2op[match_var];
				//
				old2new[i_op].match   = true;
				old2new[j_op].match   = true;
				old2new[i_op].new_var = old2new[j_op].new_var;
			}
			else
			{	size_pair = record_vv(
					var2op              ,
					op_info             ,
					old2new             ,
					i_var               ,
					play->num_par_rec() ,
					play->GetPar()      ,
					rec                 ,
					op                  ,
					arg
				);
				old2new[i_op].new_op  = size_pair.i_op;
				old2new[i_op].new_var = size_pair.i_var;
				replace_hash = true;
			}
			break;
			// ---------------------------------------------------
			// Conditional expression operators
			case CExpOp:
			CPPAD_ASSERT_NARG_NRES(op, 6, 1);
			new_arg[0] = arg[0];
			new_arg[1] = arg[1];
			mask = 1;
			for(size_t i = 2; i < 6; i++)
			{	if( arg[1] & mask )
				{	new_arg[i] = old2new[ var2op[arg[i]] ].new_var;
					CPPAD_ASSERT_UNKNOWN(
						size_t(new_arg[i]) < num_var
					);
				}
				else	new_arg[i] = rec->PutPar(
						play->GetPar( arg[i] )
				);
				mask = mask << 1;
			}
			rec->PutArg(
				new_arg[0] ,
				new_arg[1] ,
				new_arg[2] ,
				new_arg[3] ,
				new_arg[4] ,
				new_arg[5]
			);
			old2new[i_op].new_op  = rec->num_op_rec();
			old2new[i_op].new_var = rec->PutOp(op);
			//
			// The new addresses for left and right are used during
			// fill in the arguments for the CSkip operations. This does not
			// affect max_left_right which is used during this sweep.
			CPPAD_ASSERT_UNKNOWN( cskip_info_index > 0 );
			cskip_info_index--;
			cskip_info[ cskip_info_index ].left  = new_arg[2];
			cskip_info[ cskip_info_index ].right = new_arg[3];
			break;
			// ---------------------------------------------------
			// Operations with no arguments and no results
			case EndOp:
			CPPAD_ASSERT_NARG_NRES(op, 0, 0);
			old2new[i_op].new_op = rec->num_op_rec();
			rec->PutOp(op);
			break;
			// ---------------------------------------------------
			// Operations with two arguments and no results
			case LepvOp:
			case LtpvOp:
			case EqpvOp:
			case NepvOp:
			CPPAD_ASSERT_NARG_NRES(op, 2, 0);
			new_arg[0] = rec->PutPar( play->GetPar(arg[0]) );
			new_arg[1] = old2new[ var2op[arg[1]] ].new_var;
			rec->PutArg(new_arg[0], new_arg[1]);
			old2new[i_op].new_op = rec->num_op_rec();
			rec->PutOp(op);
			break;
			//
			case LevpOp:
			case LtvpOp:
			CPPAD_ASSERT_NARG_NRES(op, 2, 0);
			new_arg[0] = old2new[ var2op[arg[0]] ].new_var;
			new_arg[1] = rec->PutPar( play->GetPar(arg[1]) );
			rec->PutArg(new_arg[0], new_arg[1]);
			old2new[i_op].new_op = rec->num_op_rec();
			rec->PutOp(op);
			break;
			//
			case LevvOp:
			case LtvvOp:
			case EqvvOp:
			case NevvOp:
			CPPAD_ASSERT_NARG_NRES(op, 2, 0);
			new_arg[0] = old2new[ var2op[arg[0]] ].new_var;
			new_arg[1] = old2new[ var2op[arg[1]] ].new_var;
			rec->PutArg(new_arg[0], new_arg[1]);
			old2new[i_op].new_op = rec->num_op_rec();
			rec->PutOp(op);
			break;

			// ---------------------------------------------------
			// Operations with no arguments and one result
			case InvOp:
			CPPAD_ASSERT_NARG_NRES(op, 0, 1);
			old2new[i_op].new_op  = rec->num_op_rec();
			old2new[i_op].new_var = rec->PutOp(op);
			break;
			// ---------------------------------------------------
			// Operations with one argument that is a parameter
			case ParOp:
			CPPAD_ASSERT_NARG_NRES(op, 1, 1);
			new_arg[0] = rec->PutPar( play->GetPar(arg[0] ) );
			rec->PutArg( new_arg[0] );
			//
			old2new[i_op].new_op  = rec->num_op_rec();
			old2new[i_op].new_var = rec->PutOp(op);
			break;
			// ---------------------------------------------------
			// Load using a parameter index
			case LdpOp:
			CPPAD_ASSERT_NARG_NRES(op, 3, 1);
			new_arg[0] = new_vecad_ind[ arg[0] ];
			new_arg[1] = arg[1];
			new_arg[2] = rec->num_load_op_rec();
			CPPAD_ASSERT_UNKNOWN( size_t(new_arg[0]) < num_vecad_ind );
			rec->PutArg(
				new_arg[0],
				new_arg[1],
				new_arg[2]
			);
			old2new[i_op].new_op  = rec->num_op_rec();
			old2new[i_op].new_var = rec->PutLoadOp(op);
			break;
			// ---------------------------------------------------
			// Load using a variable index
			case LdvOp:
			CPPAD_ASSERT_NARG_NRES(op, 3, 1);
			new_arg[0] = new_vecad_ind[ arg[0] ];
			new_arg[1] = old2new[ var2op[arg[1]] ].new_var;
			new_arg[2] = rec->num_load_op_rec();
			CPPAD_ASSERT_UNKNOWN( size_t(new_arg[0]) < num_vecad_ind );
			CPPAD_ASSERT_UNKNOWN( size_t(new_arg[1]) < num_var );
			rec->PutArg(
				new_arg[0],
				new_arg[1],
				new_arg[2]
			);
			old2new[i_op].new_op  = rec->num_op_rec();
			old2new[i_op].new_var = rec->PutLoadOp(op);
			break;
			// ---------------------------------------------------
			// Store a parameter using a parameter index
			case StppOp:
			CPPAD_ASSERT_NARG_NRES(op, 3, 0);
			new_arg[0] = new_vecad_ind[ arg[0] ];
			new_arg[1] = rec->PutPar( play->GetPar(arg[1]) );
			new_arg[2] = rec->PutPar( play->GetPar(arg[2]) );
			CPPAD_ASSERT_UNKNOWN( size_t(new_arg[0]) < num_vecad_ind );
			rec->PutArg(
				new_arg[0],
				new_arg[1],
				new_arg[2]
			);
			old2new[i_op].new_op = rec->num_op_rec();
			rec->PutOp(op);
			break;
			// ---------------------------------------------------
			// Store a parameter using a variable index
			case StvpOp:
			CPPAD_ASSERT_NARG_NRES(op, 3, 0);
			new_arg[0] = new_vecad_ind[ arg[0] ];
			new_arg[1] = old2new[ var2op[arg[1]] ].new_var;
			new_arg[2] = rec->PutPar( play->GetPar(arg[2]) );
			CPPAD_ASSERT_UNKNOWN( size_t(new_arg[0]) < num_vecad_ind );
			CPPAD_ASSERT_UNKNOWN( size_t(new_arg[1]) < num_var );
			rec->PutArg(
				new_arg[0],
				new_arg[1],
				new_arg[2]
			);
			old2new[i_op].new_op = rec->num_op_rec();
			rec->PutOp(op);
			break;
			// ---------------------------------------------------
			// Store a variable using a parameter index
			case StpvOp:
			CPPAD_ASSERT_NARG_NRES(op, 3, 0);
			new_arg[0] = new_vecad_ind[ arg[0] ];
			new_arg[1] = rec->PutPar( play->GetPar(arg[1]) );
			new_arg[2] = old2new[ var2op[arg[2]] ].new_var;
			CPPAD_ASSERT_UNKNOWN( size_t(new_arg[0]) < num_vecad_ind );
			CPPAD_ASSERT_UNKNOWN( size_t(new_arg[1]) < num_var );
			CPPAD_ASSERT_UNKNOWN( size_t(new_arg[2]) < num_var );
			rec->PutArg(
				new_arg[0],
				new_arg[1],
				new_arg[2]
			);
			old2new[i_op].new_op = rec->num_op_rec();
			rec->PutOp(op);
			break;
			// ---------------------------------------------------
			// Store a variable using a variable index
			case StvvOp:
			CPPAD_ASSERT_NARG_NRES(op, 3, 0);
			new_arg[0] = new_vecad_ind[ arg[0] ];
			new_arg[1] = old2new[ var2op[arg[1]] ].new_var;
			new_arg[2] = old2new[ var2op[arg[2]] ].new_var;
			CPPAD_ASSERT_UNKNOWN( size_t(new_arg[0]) < num_vecad_ind );
			CPPAD_ASSERT_UNKNOWN( size_t(new_arg[1]) < num_var );
			CPPAD_ASSERT_UNKNOWN( size_t(new_arg[2]) < num_var );
			rec->PutArg(
				new_arg[0],
				new_arg[1],
				new_arg[2]
			);
			old2new[i_op].new_op = rec->num_op_rec();
			rec->PutOp(op);
			break;

			// -----------------------------------------------------------
			case UserOp:
			CPPAD_ASSERT_NARG_NRES(op, 4, 0);
			// user_old, user_n, user_m
			if( op_info[first_user_i_op].usage > 0 )
			{	rec->PutArg(arg[0], arg[1], arg[2], arg[3]);
				old2new[i_op].new_op = rec->num_op_rec();
				rec->PutOp(UserOp);
			}
			break;

			case UsrapOp:
			CPPAD_ASSERT_NARG_NRES(op, 1, 0);
			if( op_info[first_user_i_op].usage > 0 )
			{	new_arg[0] = rec->PutPar( play->GetPar(arg[0]) );
				rec->PutArg(new_arg[0]);
				old2new[i_op].new_op = rec->num_op_rec();
				rec->PutOp(UsrapOp);
			}
			break;

			case UsravOp:
			CPPAD_ASSERT_NARG_NRES(op, 1, 0);
			if( op_info[first_user_i_op].usage > 0 )
			{	new_arg[0] = old2new[ var2op[arg[0]] ].new_var;
				if( size_t(new_arg[0]) < num_var )
				{	rec->PutArg(new_arg[0]);
					old2new[i_op].new_op = rec->num_op_rec();
					rec->PutOp(UsravOp);
				}
				else
				{	// This argument does not affect the result and
					// has been optimized out so use nan in its place.
					new_arg[0] = rec->PutPar( base_nan );
					rec->PutArg(new_arg[0]);
					old2new[i_op].new_op = rec->num_op_rec();
					rec->PutOp(UsrapOp);
				}
			}
			break;

			case UsrrpOp:
			CPPAD_ASSERT_NARG_NRES(op, 1, 0);
			if( op_info[first_user_i_op].usage > 0 )
			{	new_arg[0] = rec->PutPar( play->GetPar(arg[0]) );
				rec->PutArg(new_arg[0]);
				old2new[i_op].new_op = rec->num_op_rec();
				rec->PutOp(UsrrpOp);
			}
			break;

			case UsrrvOp:
			CPPAD_ASSERT_NARG_NRES(op, 0, 1);
			if( op_info[first_user_i_op].usage > 0 )
			{	old2new[i_op].new_op  = rec->num_op_rec();
				old2new[i_op].new_var = rec->PutOp(UsrrvOp);
			}
			break;
			// ---------------------------------------------------

			// all cases should be handled above
			default:
			CPPAD_ASSERT_UNKNOWN(false);

		}
		if( replace_hash )
		{	// The old variable index i_var corresponds to the
			// new variable index old2new[var2op[i_var]].new_var. In addition
			// this is the most recent variable that has this code.
			hash_table_var[code] = i_var;
		}

	}
	// modify the dependent variable vector to new indices
	for(size_t i = 0; i < dep_taddr.size(); i++ )
	{	dep_taddr[i] = old2new[ var2op[dep_taddr[i]] ].new_var;
		CPPAD_ASSERT_UNKNOWN( size_t(dep_taddr[i]) < num_var );
	}

# ifndef NDEBUG
	for(i_op = 0; i_op < num_op; i_op++)
		if( NumRes( op_info[i_op].op ) > 0 )
			CPPAD_ASSERT_UNKNOWN( old2new[i_op].new_op < rec->num_op_rec() );
# endif
	// fill in the arguments for the CSkip operations
	CPPAD_ASSERT_UNKNOWN( cskip_order_next == cskip_info.size() );
	for(size_t i = 0; i < cskip_info.size(); i++)
	{	struct_cskip_info info = cskip_info[i];
		if( info.i_arg > 0 )
		{
			size_t n_true  = info.skip_old_op_true.size();
			size_t n_false = info.skip_old_op_false.size();
			size_t i_arg   = info.i_arg;
			rec->ReplaceArg(i_arg++, info.cop   );
			rec->ReplaceArg(i_arg++, info.flag  );
			rec->ReplaceArg(i_arg++, info.left  );
			rec->ReplaceArg(i_arg++, info.right );
			rec->ReplaceArg(i_arg++, n_true     );
			rec->ReplaceArg(i_arg++, n_false    );
			for(size_t j = 0; j < info.skip_old_op_true.size(); j++)
			{	i_op = info.skip_old_op_true[j];
				bool remove = old2new[i_op].new_op == 0;
				if( old2new[i_op].match )
					remove = true;
				if( remove )
				{	// This operation has been removed or matched,
					// so use an operator index that never comes up.
					rec->ReplaceArg(i_arg++, rec->num_op_rec());
				}
				else
					rec->ReplaceArg(i_arg++, old2new[i_op].new_op );
			}
			for(size_t j = 0; j < info.skip_old_op_false.size(); j++)
			{	i_op   = info.skip_old_op_false[j];
				bool remove = old2new[i_op].new_op == 0;
				if( old2new[i_op].match )
					remove = true;
				if( remove )
				{	// This operation has been removed or matched,
					// so use an operator index that never comes up.
					rec->ReplaceArg(i_arg++, rec->num_op_rec());
				}
				else
					rec->ReplaceArg(i_arg++, old2new[i_op].new_op );
			}
			rec->ReplaceArg(i_arg++, n_true + n_false);
# ifndef NDEBUG
			size_t n_arg   = 7 + n_true + n_false;
			CPPAD_ASSERT_UNKNOWN( info.i_arg + n_arg == i_arg );
# endif
		}
	}
}

} } } // END_CPPAD_LOCAL_OPTIMIZE_NAMESPACE

# endif
# ifndef CPPAD_LOCAL_SWEEP_FORWARD2_HPP
# define CPPAD_LOCAL_SWEEP_FORWARD2_HPP

/* --------------------------------------------------------------------------
CppAD: C++ Algorithmic Differentiation: Copyright (C) 2003-18 Bradley M. Bell

CppAD is distributed under multiple licenses. This distribution is under
the terms of the
                    Eclipse Public License Version 1.0.

A copy of this license is included in the COPYING file of this distribution.
Please visit http://www.coin-or.org/CppAD/ for information on other licenses.
-------------------------------------------------------------------------- */

# include <cppad/local/play/user_op_info.hpp>

// BEGIN_CPPAD_LOCAL_SWEEP_NAMESPACE
namespace CppAD { namespace local { namespace sweep {
/*!
\file sweep/forward2.hpp
Compute one Taylor coefficient for each direction requested.
*/

/*
\def CPPAD_ATOMIC_CALL
This avoids warnings when NDEBUG is defined and user_ok is not used.
If NDEBUG is defined, this resolves to
\code
	user_atom->forward
\endcode
otherwise, it respolves to
\code
	user_ok = user_atom->forward
\endcode
This macro is undefined at the end of this file to facillitate its
use with a different definition in other files.
*/
# ifdef NDEBUG
# define CPPAD_ATOMIC_CALL user_atom->forward
# else
# define CPPAD_ATOMIC_CALL user_ok = user_atom->forward
# endif

/*!
\def CPPAD_FORWARD2_TRACE
This value is either zero or one.
Zero is the normal operational value.
If it is one, a trace of every forward2sweep computation is printed.
*/
# define CPPAD_FORWARD2_TRACE 0

/*!
Compute multiple directions forward mode Taylor coefficients.

\tparam Base
The type used during the forward mode computations; i.e., the corresponding
recording of operations used the type AD<Base>.

\param q
is the order of the Taylor coefficients
that are computed during this call;
<code>q > 0</code>.

\param r
is the number of Taylor coefficients
that are computed during this call.

\param n
is the number of independent variables on the tape.

\param numvar
is the total number of variables on the tape.
This is also equal to the number of rows in the matrix taylor; i.e.,
play->num_var_rec().

\param play
The information stored in play
is a recording of the operations corresponding to the function
\f[
	F : {\bf R}^n \rightarrow {\bf R}^m
\f]
where \f$ n \f$ is the number of independent variables and
\f$ m \f$ is the number of dependent variables.

\param J
Is the number of columns in the coefficient matrix taylor.
This must be greater than or equal one.

\param taylor
\n
\b Input:
For <code>i = 1 , ... , numvar-1</code>,
<code>taylor[ (J-1)*r*i + i + 0 ]</code>
is the zero order Taylor coefficient corresponding to
the i-th variable and all directions.
For <code>i = 1 , ... , numvar-1</code>,
For <code>k = 1 , ... , q-1</code>,
<code>ell = 0 , ... , r-1</code>,
<code>taylor[ (J-1)*r*i + i + (k-1)*r + ell + 1 ]</code>
is the k-th order Taylor coefficient corresponding to
the i-th variabel and ell-th direction.
\n
\n
\b Input:
For <code>i = 1 , ... , n</code>,
<code>ell = 0 , ... , r-1</code>,
<code>taylor[ (J-1)*r*i + i + (q-1)*r + ell + 1 ]</code>
is the q-th order Taylor coefficient corresponding to
the i-th variable and ell-th direction
(these are the independent varaibles).
\n
\n
\b Output:
For <code>i = n+1 , ... , numvar-1</code>,
<code>ell = 0 , ... , r-1</code>,
<code>taylor[ (J-1)*r*i + i + (q-1)*r + ell + 1 ]</code>
is the q-th order Taylor coefficient corresponding to
the i-th variable and ell-th direction.

\param cskip_op
Is a vector with size play->num_op_rec().
If cskip_op[i] is true, the operator with index i
does not affect any of the dependent variable (given the value
of the independent variables).

\param var_by_load_op
is a vector with size play->num_load_op_rec().
It is the variable index corresponding to each the
load instruction.
In the case where the index is zero,
the instruction corresponds to a parameter (not variable).

\param not_used_rec_base
Specifies RecBase for this call.

*/

template <class Addr, class Base, class RecBase>
void forward2(
	const local::player<Base>*  play,
	const size_t                q,
	const size_t                r,
	const size_t                n,
	const size_t                numvar,
	const size_t                J,
	Base*                       taylor,
	const bool*                 cskip_op,
	const pod_vector<Addr>&     var_by_load_op,
	const RecBase&              not_used_rec_base
)
{
	CPPAD_ASSERT_UNKNOWN( q > 0 );
	CPPAD_ASSERT_UNKNOWN( J >= q + 1 );
	CPPAD_ASSERT_UNKNOWN( play->num_var_rec() == numvar );

	// used to avoid compiler errors until all operators are implemented
	size_t p = q;

	// work space used by UserOp.
	vector<bool> user_vx;        // empty vecotor
	vector<bool> user_vy;        // empty vecotor
	vector<Base> user_tx_one;    // argument vector Taylor coefficients
	vector<Base> user_tx_all;
	vector<Base> user_ty_one;    // result vector Taylor coefficients
	vector<Base> user_ty_all;
	//
	// information defined by forward_user
	size_t user_old=0, user_m=0, user_n=0, user_i=0, user_j=0;
	enum_user_state user_state = start_user; // proper initialization
	//
	atomic_base<RecBase>* user_atom = CPPAD_NULL; // user's atomic op
# ifndef NDEBUG
	bool                  user_ok   = false;      // atomic op return value
# endif

	// length of the parameter vector (used by CppAD assert macros)
	const size_t num_par = play->num_par_rec();

	// pointer to the beginning of the parameter vector
	const Base* parameter = CPPAD_NULL;
	if( num_par > 0 )
		parameter = play->GetPar();

	// temporary indices
	size_t i, j, k, ell;

	// number of orders for this user calculation
	// (not needed for order zero)
	const size_t user_q1 = q+1;

	// variable indices for results vector
	// (done differently for order zero).
	vector<size_t> user_iy;

	// skip the BeginOp at the beginning of the recording
	play::const_sequential_iterator itr = play->begin();
	// op_info
	OpCode op;
	size_t i_var;
	const Addr*   arg;
	itr.op_info(op, arg, i_var);
	CPPAD_ASSERT_UNKNOWN( op == BeginOp );
# if CPPAD_FORWARD2_TRACE
	bool user_trace  = false;
	std::cout << std::endl;
	CppAD::vector<Base> Z_vec(q+1);
# endif
	bool flag; // a temporary flag to use in switch cases
	bool more_operators = true;
	while(more_operators)
	{
		// next op
		(++itr).op_info(op, arg, i_var);
		CPPAD_ASSERT_UNKNOWN( itr.op_index() < play->num_op_rec() );

		// check if we are skipping this operation
		while( cskip_op[itr.op_index()] )
		{	switch(op)
			{
				case UserOp:
				{	// get information for this user atomic call
					CPPAD_ASSERT_UNKNOWN( user_state == start_user );
					play::user_op_info<Base>(op, arg, user_old, user_m, user_n);
					//
					// skip to the second UserOp
					for(i = 0; i < user_m + user_n + 1; ++i)
						++itr;
# ifndef NDEBUG
					itr.op_info(op, arg, i_var);
					CPPAD_ASSERT_UNKNOWN( op == UserOp );
# endif
				}
				break;

				case CSkipOp:
				case CSumOp:
				itr.correct_before_increment();
				break;

				default:
				break;
			}
			(++itr).op_info(op, arg, i_var);
		}

		// action depends on the operator
		switch( op )
		{
			case AbsOp:
			forward_abs_op_dir(q, r, i_var, size_t(arg[0]), J, taylor);
			break;
			// -------------------------------------------------

			case AddvvOp:
			forward_addvv_op_dir(q, r, i_var, arg, parameter, J, taylor);
			break;
			// -------------------------------------------------

			case AddpvOp:
			CPPAD_ASSERT_UNKNOWN( size_t(arg[0]) < num_par );
			forward_addpv_op_dir(q, r, i_var, arg, parameter, J, taylor);
			break;
			// -------------------------------------------------

			case AcosOp:
			// sqrt(1 - x * x), acos(x)
			CPPAD_ASSERT_UNKNOWN( i_var < numvar  );
			forward_acos_op_dir(q, r, i_var, size_t(arg[0]), J, taylor);
			break;
			// -------------------------------------------------

# if CPPAD_USE_CPLUSPLUS_2011
			case AcoshOp:
			// sqrt(x * x - 1), acosh(x)
			CPPAD_ASSERT_UNKNOWN( i_var < numvar  );
			forward_acosh_op_dir(q, r, i_var, size_t(arg[0]), J, taylor);
			break;
# endif
			// -------------------------------------------------

			case AsinOp:
			// sqrt(1 - x * x), asin(x)
			CPPAD_ASSERT_UNKNOWN( i_var < numvar  );
			forward_asin_op_dir(q, r, i_var, size_t(arg[0]), J, taylor);
			break;
			// -------------------------------------------------

# if CPPAD_USE_CPLUSPLUS_2011
			case AsinhOp:
			// sqrt(1 + x * x), asinh(x)
			CPPAD_ASSERT_UNKNOWN( i_var < numvar  );
			forward_asinh_op_dir(q, r, i_var, size_t(arg[0]), J, taylor);
			break;
# endif
			// -------------------------------------------------

			case AtanOp:
			// 1 + x * x, atan(x)
			CPPAD_ASSERT_UNKNOWN( i_var < numvar  );
			forward_atan_op_dir(q, r, i_var, size_t(arg[0]), J, taylor);
			break;
			// -------------------------------------------------

# if CPPAD_USE_CPLUSPLUS_2011
			case AtanhOp:
			// 1 - x * x, atanh(x)
			CPPAD_ASSERT_UNKNOWN( i_var < numvar  );
			forward_atanh_op_dir(q, r, i_var, size_t(arg[0]), J, taylor);
			break;
# endif
			// -------------------------------------------------

			case CExpOp:
			forward_cond_op_dir(
				q, r, i_var, arg, num_par, parameter, J, taylor
			);
			break;
			// ---------------------------------------------------

			case CosOp:
			// sin(x), cos(x)
			CPPAD_ASSERT_UNKNOWN( i_var < numvar  );
			forward_cos_op_dir(q, r, i_var, size_t(arg[0]), J, taylor);
			break;
			// ---------------------------------------------------

			case CoshOp:
			// sinh(x), cosh(x)
			CPPAD_ASSERT_UNKNOWN( i_var < numvar  );
			forward_cosh_op_dir(q, r, i_var, size_t(arg[0]), J, taylor);
			break;
			// -------------------------------------------------

			case CSkipOp:
			// CSkipOp only does somthing on order zero.
			CPPAD_ASSERT_UNKNOWN( p > 0 );
			itr.correct_before_increment();
			break;
			// -------------------------------------------------

			case CSumOp:
			forward_csum_op_dir(
				q, r, i_var, arg, num_par, parameter, J, taylor
			);
			itr.correct_before_increment();
			break;
			// -------------------------------------------------

			case DisOp:
			forward_dis_op(p, q, r, i_var, arg, J, taylor);
			break;
			// -------------------------------------------------

			case DivvvOp:
			forward_divvv_op_dir(q, r, i_var, arg, parameter, J, taylor);
			break;
			// -------------------------------------------------

			case DivpvOp:
			CPPAD_ASSERT_UNKNOWN( size_t(arg[0]) < num_par );
			forward_divpv_op_dir(q, r, i_var, arg, parameter, J, taylor);
			break;
			// -------------------------------------------------

			case DivvpOp:
			CPPAD_ASSERT_UNKNOWN( size_t(arg[1]) < num_par );
			forward_divvp_op_dir(q, r, i_var, arg, parameter, J, taylor);
			break;
			// -------------------------------------------------

			case EndOp:
			// needed for sparse_jacobian test
			CPPAD_ASSERT_NARG_NRES(op, 0, 0);
			more_operators = false;
			break;
			// -------------------------------------------------

# if CPPAD_USE_CPLUSPLUS_2011
			case ErfOp:
			forward_erf_op_dir(q, r, i_var, arg, parameter, J, taylor);
			break;
			// -------------------------------------------------
# endif

			case ExpOp:
			forward_exp_op_dir(q, r, i_var, size_t(arg[0]), J, taylor);
			break;
			// -------------------------------------------------

# if CPPAD_USE_CPLUSPLUS_2011
			case Expm1Op:
			forward_expm1_op_dir(q, r, i_var, size_t(arg[0]), J, taylor);
			break;
# endif
			// -------------------------------------------------

			case InvOp:
			CPPAD_ASSERT_NARG_NRES(op, 0, 1);
			break;
			// -------------------------------------------------

			case LdpOp:
			case LdvOp:
			forward_load_op(
				play,
				op,
				p,
				q,
				r,
				J,
				i_var,
				arg,
				var_by_load_op.data(),
				taylor
			);
			break;
			// ---------------------------------------------------

			case EqppOp:
			case EqpvOp:
			case EqvvOp:
			case LtppOp:
			case LtpvOp:
			case LtvpOp:
			case LtvvOp:
			case LeppOp:
			case LepvOp:
			case LevpOp:
			case LevvOp:
			case NeppOp:
			case NepvOp:
			case NevvOp:
			CPPAD_ASSERT_UNKNOWN(q > 0 );
			break;
			// -------------------------------------------------

			case LogOp:
			forward_log_op_dir(q, r, i_var, size_t(arg[0]), J, taylor);
			break;
			// ---------------------------------------------------

# if CPPAD_USE_CPLUSPLUS_2011
			case Log1pOp:
			forward_log1p_op_dir(q, r, i_var, size_t(arg[0]), J, taylor);
			break;
# endif
			// ---------------------------------------------------

			case MulpvOp:
			CPPAD_ASSERT_UNKNOWN( size_t(arg[0]) < num_par );
			forward_mulpv_op_dir(q, r, i_var, arg, parameter, J, taylor);
			break;
			// -------------------------------------------------

			case MulvvOp:
			forward_mulvv_op_dir(q, r, i_var, arg, parameter, J, taylor);
			break;
			// -------------------------------------------------

			case ParOp:
			k = i_var*(J-1)*r + i_var + (q-1)*r + 1;
			for(ell = 0; ell < r; ell++)
				taylor[k + ell] = Base(0.0);
			break;
			// -------------------------------------------------

			case PowpvOp:
			CPPAD_ASSERT_UNKNOWN( size_t(arg[0]) < num_par );
			forward_powpv_op_dir(q, r, i_var, arg, parameter, J, taylor);
			break;
			// -------------------------------------------------

			case PowvpOp:
			CPPAD_ASSERT_UNKNOWN( size_t(arg[1]) < num_par );
			forward_powvp_op_dir(q, r, i_var, arg, parameter, J, taylor);
			break;
			// -------------------------------------------------

			case PowvvOp:
			forward_powvv_op_dir(q, r, i_var, arg, parameter, J, taylor);
			break;
			// -------------------------------------------------

			case PriOp:
			CPPAD_ASSERT_UNKNOWN(q > 0);
			break;
			// -------------------------------------------------

			case SignOp:
			// sign(x)
			CPPAD_ASSERT_UNKNOWN( i_var < numvar  );
			forward_sign_op_dir(q, r, i_var, size_t(arg[0]), J, taylor);
			break;
			// -------------------------------------------------

			case SinOp:
			// cos(x), sin(x)
			CPPAD_ASSERT_UNKNOWN( i_var < numvar  );
			forward_sin_op_dir(q, r, i_var, size_t(arg[0]), J, taylor);
			break;
			// -------------------------------------------------

			case SinhOp:
			// cosh(x), sinh(x)
			CPPAD_ASSERT_UNKNOWN( i_var < numvar  );
			forward_sinh_op_dir(q, r, i_var, size_t(arg[0]), J, taylor);
			break;
			// -------------------------------------------------

			case SqrtOp:
			forward_sqrt_op_dir(q, r, i_var, size_t(arg[0]), J, taylor);
			break;
			// -------------------------------------------------

			case StppOp:
			case StpvOp:
			case StvpOp:
			case StvvOp:
			CPPAD_ASSERT_UNKNOWN(q > 0 );
			break;
			// -------------------------------------------------

			case SubvvOp:
			forward_subvv_op_dir(q, r, i_var, arg, parameter, J, taylor);
			break;
			// -------------------------------------------------

			case SubpvOp:
			CPPAD_ASSERT_UNKNOWN( size_t(arg[0]) < num_par );
			forward_subpv_op_dir(q, r, i_var, arg, parameter, J, taylor);
			break;
			// -------------------------------------------------

			case SubvpOp:
			CPPAD_ASSERT_UNKNOWN( size_t(arg[1]) < num_par );
			forward_subvp_op_dir(q, r, i_var, arg, parameter, J, taylor);
			break;
			// -------------------------------------------------

			case TanOp:
			// tan(x)^2, tan(x)
			CPPAD_ASSERT_UNKNOWN( i_var < numvar  );
			forward_tan_op_dir(q, r, i_var, size_t(arg[0]), J, taylor);
			break;
			// -------------------------------------------------

			case TanhOp:
			// tanh(x)^2, tanh(x)
			CPPAD_ASSERT_UNKNOWN( i_var < numvar  );
			forward_tanh_op_dir(q, r, i_var, size_t(arg[0]), J, taylor);
			break;
			// -------------------------------------------------

			case UserOp:
			// start or end an atomic function call
			flag = user_state == start_user;
			user_atom = play::user_op_info<RecBase>(
				op, arg, user_old, user_m, user_n
			);
			if( flag )
			{	user_state = arg_user;
				user_i     = 0;
				user_j     = 0;
				//
				user_tx_one.resize(user_n * user_q1);
				user_tx_all.resize(user_n * (q * r + 1));
				//
				user_ty_one.resize(user_m * user_q1);
				user_ty_all.resize(user_m * (q * r + 1));
				//
				user_iy.resize(user_m);
			}
			else
			{	user_state = start_user;
				//
				// call users function for this operation
				user_atom->set_old(user_old);
				for(ell = 0; ell < r; ell++)
				{	// set user_tx
					for(j = 0; j < user_n; j++)
					{	size_t j_all     = j * (q * r + 1);
						size_t j_one     = j * user_q1;
						user_tx_one[j_one+0] = user_tx_all[j_all+0];
						for(k = 1; k < user_q1; k++)
						{	size_t k_all       = j_all + (k-1)*r+1+ell;
							size_t k_one       = j_one + k;
							user_tx_one[k_one] = user_tx_all[k_all];
						}
					}
					// set user_ty
					for(i = 0; i < user_m; i++)
					{	size_t i_all     = i * (q * r + 1);
						size_t i_one     = i * user_q1;
						user_ty_one[i_one+0] = user_ty_all[i_all+0];
						for(k = 1; k < q; k++)
						{	size_t k_all       = i_all + (k-1)*r+1+ell;
							size_t k_one       = i_one + k;
							user_ty_one[k_one] = user_ty_all[k_all];
						}
					}
					CPPAD_ATOMIC_CALL(
					q, q, user_vx, user_vy, user_tx_one, user_ty_one
					);
# ifndef NDEBUG
					if( ! user_ok )
					{	std::string msg =
							user_atom->afun_name()
							+ ": atomic_base.forward: returned false";
						CPPAD_ASSERT_KNOWN(false, msg.c_str() );
					}
# endif
					for(i = 0; i < user_m; i++)
					{	if( user_iy[i] > 0 )
						{	size_t i_taylor = user_iy[i]*((J-1)*r+1);
							size_t q_taylor = i_taylor + (q-1)*r+1+ell;
							size_t q_one    = i * user_q1 + q;
							taylor[q_taylor] = user_ty_one[q_one];
						}
					}
				}
# if CPPAD_FORWARD2_TRACE
				user_trace = true;
# endif
			}
			break;

			case UsrapOp:
			// parameter argument for a user atomic function
			CPPAD_ASSERT_UNKNOWN( NumArg(op) == 1 );
			CPPAD_ASSERT_UNKNOWN( user_state == arg_user );
			CPPAD_ASSERT_UNKNOWN( user_i == 0 );
			CPPAD_ASSERT_UNKNOWN( user_j < user_n );
			CPPAD_ASSERT_UNKNOWN( size_t( arg[0] ) < num_par );
			//
			user_tx_all[user_j*(q*r+1) + 0] = parameter[ arg[0]];
			for(ell = 0; ell < r; ell++)
				for(k = 1; k < user_q1; k++)
					user_tx_all[user_j*(q*r+1) + (k-1)*r+1+ell] = Base(0.0);
			//
			++user_j;
			if( user_j == user_n )
				user_state = ret_user;
			break;

			case UsravOp:
			// variable argument for a user atomic function
			CPPAD_ASSERT_UNKNOWN( NumArg(op) == 1 );
			CPPAD_ASSERT_UNKNOWN( user_state == arg_user );
			CPPAD_ASSERT_UNKNOWN( user_i == 0 );
			CPPAD_ASSERT_UNKNOWN( user_j < user_n );
			//
			user_tx_all[user_j*(q*r+1)+0] = taylor[size_t(arg[0])*((J-1)*r+1)+0];
			for(ell = 0; ell < r; ell++)
			{	for(k = 1; k < user_q1; k++)
				{	user_tx_all[user_j*(q*r+1) + (k-1)*r+1+ell] =
						taylor[size_t(arg[0])*((J-1)*r+1) + (k-1)*r+1+ell];
				}
			}
			//
			++user_j;
			if( user_j == user_n )
				user_state = ret_user;
			break;

			case UsrrpOp:
			// parameter result for a user atomic function
			CPPAD_ASSERT_NARG_NRES(op, 1, 0);
			CPPAD_ASSERT_UNKNOWN( user_state == ret_user );
			CPPAD_ASSERT_UNKNOWN( user_i < user_m );
			CPPAD_ASSERT_UNKNOWN( user_j == user_n );
			//
			user_iy[user_i] = 0;
			user_ty_all[user_i*(q*r+1) + 0] = parameter[ arg[0]];
			for(ell = 0; ell < r; ell++)
				for(k = 1; k < user_q1; k++)
					user_ty_all[user_i*(q*r+1) + (k-1)*r+1+ell] = Base(0.0);
			//
			++user_i;
			if( user_i == user_m )
				user_state = end_user;
			break;

			case UsrrvOp:
			// variable result for a user atomic function
			CPPAD_ASSERT_NARG_NRES(op, 0, 1);
			CPPAD_ASSERT_UNKNOWN( user_state == ret_user );
			CPPAD_ASSERT_UNKNOWN( user_i < user_m );
			CPPAD_ASSERT_UNKNOWN( user_j == user_n );
			//
			user_iy[user_i] = i_var;
			user_ty_all[user_i*(q*r+1)+0] = taylor[i_var*((J-1)*r+1)+0];
			for(ell = 0; ell < r; ell++)
			{	for(k = 1; k < user_q1; k++)
				{	user_ty_all[user_i*(q*r+1) + (k-1)*r+1+ell] =
						taylor[i_var*((J-1)*r+1) + (k-1)*r+1+ell];
				}
			}
			++user_i;
			if( user_i == user_m )
				user_state = end_user;
			break;
			// -------------------------------------------------

			case ZmulpvOp:
			CPPAD_ASSERT_UNKNOWN( size_t(arg[0]) < num_par );
			forward_zmulpv_op_dir(q, r, i_var, arg, parameter, J, taylor);
			break;
			// -------------------------------------------------

			case ZmulvpOp:
			CPPAD_ASSERT_UNKNOWN( size_t(arg[1]) < num_par );
			forward_zmulvp_op_dir(q, r, i_var, arg, parameter, J, taylor);
			break;
			// -------------------------------------------------

			case ZmulvvOp:
			forward_zmulvv_op_dir(q, r, i_var, arg, parameter, J, taylor);
			break;
			// -------------------------------------------------

			default:
			CPPAD_ASSERT_UNKNOWN(0);
		}
# if CPPAD_FORWARD2_TRACE
		if( user_trace )
		{	user_trace = false;
			CPPAD_ASSERT_UNKNOWN( op == UserOp );
			CPPAD_ASSERT_UNKNOWN( NumArg(UsrrvOp) == 0 );
			for(i = 0; i < user_m; i++) if( user_iy[i] > 0 )
			{	size_t i_tmp   = (itr.op_index() + i) - user_m;
				printOp(
					std::cout,
					play,
					i_tmp,
					user_iy[i],
					UsrrvOp,
					CPPAD_NULL
				);
				Base* Z_tmp = taylor + user_iy[i]*((J-1) * r + 1);
				{	Z_vec[0]    = Z_tmp[0];
					for(ell = 0; ell < r; ell++)
					{	std::cout << std::endl << "     ";
						for(size_t p_tmp = 1; p_tmp <= q; p_tmp++)
							Z_vec[p_tmp] = Z_tmp[(p_tmp-1)*r+ell+1];
						printOpResult(
							std::cout,
							q + 1,
							Z_vec.data(),
							0,
							(Base *) CPPAD_NULL
						);
					}
				}
				std::cout << std::endl;
			}
		}
		if( op != UsrrvOp )
		{	printOp(
				std::cout,
				play,
				itr.op_index(),
				i_var,
				op,
				arg
			);
			Base* Z_tmp = CPPAD_NULL;
			if( op == UsravOp )
				Z_tmp = taylor + size_t(arg[0])*((J-1) * r + 1);
			else if( NumRes(op) > 0 )
				Z_tmp = taylor + i_var*((J-1)*r + 1);
			if( Z_tmp != CPPAD_NULL )
			{	Z_vec[0]    = Z_tmp[0];
				for(ell = 0; ell < r; ell++)
				{	std::cout << std::endl << "     ";
					for(size_t p_tmp = 1; p_tmp <= q; p_tmp++)
						Z_vec[p_tmp] = Z_tmp[ (p_tmp-1)*r + ell + 1];
					printOpResult(
						std::cout,
						q + 1,
						Z_vec.data(),
						0,
						(Base *) CPPAD_NULL
					);
				}
			}
			std::cout << std::endl;
		}
	}
	std::cout << std::endl;
# else
	}
# endif
	CPPAD_ASSERT_UNKNOWN( user_state == start_user );

	return;
}

// preprocessor symbols that are local to this file
# undef CPPAD_FORWARD2_TRACE
# undef CPPAD_ATOMIC_CALL

} } } // END_CPPAD_LOCAL_SWEEP_NAMESPACE
# endif

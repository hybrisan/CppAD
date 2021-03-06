# ifndef CPPAD_CORE_CON_DYN_VAR_HPP
# define CPPAD_CORE_CON_DYN_VAR_HPP

/* --------------------------------------------------------------------------
CppAD: C++ Algorithmic Differentiation: Copyright (C) 2003-18 Bradley M. Bell

CppAD is distributed under multiple licenses. This distribution is under
the terms of the
                    Eclipse Public License Version 1.0.

A copy of this license is included in the COPYING file of this distribution.
Please visit http://www.coin-or.org/CppAD/ for information on other licenses.
-------------------------------------------------------------------------- */
/*
---------------------------------------------------------------------------

$begin con_dyn_var$$
$spell
	VecAD
	const
	bool
$$

$section Constant, Dynamic, Parameter, and Variable$$

$head Syntax$$
$icode%b% = Constant(%x%)
%$$
$icode%b% = Dynamic(%x%)
%$$
$icode%b% = Parameter(%x%)
%$$
$icode%b% = Variable(%x%)
%$$

$head x$$
The argument $icode x$$ has prototype
$codei%
	const AD<%Base%>    &%x%
	const VecAD<%Base%> &%x%
%$$

$head b$$
The return value $icode b$$ has prototype
$codei%
	bool %b%
%$$

$head Constant$$
The return value for $code Constant$$ is true
is true if and only if $icode x$$ is
a $cref/constant/glossary/Parameter/Constant/$$ parameter.
A $cref/VecAD<Base>/VecAD/$$ object is a constant parameter
if no element of the vector depends on the independent variables.

$head Dynamic$$
The return value for $code Dynamic$$ is true
is true if and only if $icode x$$ is
a $cref/dynamic/glossary/Parameter/Dynamic/$$ parameter.
No element of a $cref/VecAD<Base>/VecAD/$$ object
can depend on the dynamic parameters and this function returns false
for these objects.

$head Parameter$$
The return value for $code Parameter$$ is true
is true if and only if $icode x$$ is
a $cref/parameter/glossary/Parameter/$$.
A $cref/VecAD<Base>/VecAD/$$ object is a parameter
if no element of the vector depends on the independent variables.

$head Variable$$
The return value for $code Variable$$ is true
is true if and only if $icode x$$ is
a $cref/variable/glossary/Variable/$$.
A $cref/VecAD<Base>/VecAD/$$ object is a variable
if any element of the vector depends on the independent variables.

$head Operation Sequence$$
The result of this operation is not an
$cref/AD of Base/glossary/AD of Base/$$ object.
Thus it will not be recorded as part of an
AD of $icode Base$$
$cref/operation sequence/glossary/Operation/Sequence/$$.

$head Example$$
$children%
	example/general/con_dyn_var.cpp
%$$
The file
$cref con_dyn_var.cpp$$
contains an example and test of these functions.
It returns true if it succeeds and false otherwise.

$end
-----------------------------------------------------------------------------
*/

namespace CppAD {
	// -----------------------------------------------------------------------
	// Constant
	template <class Base>
	bool Constant(const AD<Base> &x)
	{	CPPAD_ASSERT_UNKNOWN( x.tape_id_==0 || x.ad_type_!=local::no_ad_type );
		if( x.tape_id_ == 0 )
			return true;
		//
		size_t thread = size_t(x.tape_id_ % CPPAD_MAX_NUM_THREADS);
		return x.tape_id_ != *AD<Base>::tape_id_ptr(thread);
	}
	template <class Base>
	bool Constant(const VecAD<Base> &x)
	{	return Parameter(x); }

	// -----------------------------------------------------------------------
	// Dynamic
	template <class Base>
	bool Dynamic(const AD<Base> &x)
	{	CPPAD_ASSERT_UNKNOWN( x.tape_id_==0 || x.ad_type_!=local::no_ad_type );
		if( (x.tape_id_ == 0) | (x.ad_type_ != local::dyn_ad_type) )
			return false;
		//
		size_t thread = size_t(x.tape_id_ % CPPAD_MAX_NUM_THREADS);
		return x.tape_id_ == *AD<Base>::tape_id_ptr(thread);
	}
	template <class Base>
	bool Dynamic(const VecAD<Base> &x)
	{	return false; }

	// -----------------------------------------------------------------------
	// Parameter
	template <class Base>
	bool Parameter(const AD<Base> &x)
	{	CPPAD_ASSERT_UNKNOWN( x.tape_id_==0 || x.ad_type_!=local::no_ad_type );
		if( (x.tape_id_ == 0) | (x.ad_type_ == local::dyn_ad_type) )
			return true;
		//
		size_t thread = size_t(x.tape_id_ % CPPAD_MAX_NUM_THREADS);
		return x.tape_id_ != *AD<Base>::tape_id_ptr(thread);
	}
	template <class Base>
	bool Parameter(const VecAD<Base> &x)
	{	if( x.tape_id_ == 0 )
			return true;
		//
		size_t thread = size_t(x.tape_id_ % CPPAD_MAX_NUM_THREADS);
		return x.tape_id_ != *AD<Base>::tape_id_ptr(thread);
	}

	// -----------------------------------------------------------------------
	// Variable
	template <class Base>
	bool Variable(const AD<Base> &x)
	{	CPPAD_ASSERT_UNKNOWN( x.tape_id_==0 || x.ad_type_!=local::no_ad_type );
		if( (x.tape_id_ == 0) | (x.ad_type_ != local::var_ad_type) )
			return false;
		//
		size_t thread = size_t(x.tape_id_ % CPPAD_MAX_NUM_THREADS);
		return x.tape_id_ == *AD<Base>::tape_id_ptr(thread);
	}
	template <class Base>
	bool Variable(const VecAD<Base> &x)
	{	if( x.tape_id_ == 0 )
			return false;
		//
		size_t thread = size_t(x.tape_id_ % CPPAD_MAX_NUM_THREADS);
		return x.tape_id_ == *AD<Base>::tape_id_ptr(thread);
	}
}
// END CppAD namespace


# endif

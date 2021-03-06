/* --------------------------------------------------------------------------
CppAD: C++ Algorithmic Differentiation: Copyright (C) 2003-18 Bradley M. Bell

CppAD is distributed under multiple licenses. This distribution is under
the terms of the
                    Eclipse Public License Version 1.0.

A copy of this license is included in the COPYING file of this distribution.
Please visit http://www.coin-or.org/CppAD/ for information on other licenses.
-------------------------------------------------------------------------- */
# include <cppad/cppad.hpp>

// implicit constructor from double
bool implicit_ctor(void)
{	using CppAD::AD;
	bool ok = true;
	//
	AD< AD<double> > x = 5.0;
	ok &= Value(x) == 5.0;
	//
	return ok;
}

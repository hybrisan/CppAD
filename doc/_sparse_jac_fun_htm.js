var list_across0 = [
'_contents.htm',
'_reference.htm',
'_index.htm',
'_search.htm',
'_external.htm'
];
var list_up0 = [
'cppad.htm',
'speed.htm',
'speed_utility.htm',
'sparse_jac_fun.htm'
];
var list_down3 = [
'install.htm',
'theory.htm',
'ad.htm',
'adfun.htm',
'preprocessor.htm',
'multi_thread.htm',
'utility.htm',
'ipopt_solve.htm',
'example.htm',
'speed.htm',
'appendix.htm'
];
var list_down2 = [
'speed_main.htm',
'speed_utility.htm',
'speed_double.htm',
'speed_adolc.htm',
'speed_cppad.htm',
'speed_fadbad.htm',
'speed_sacado.htm'
];
var list_down1 = [
'det_by_lu.htm',
'det_of_minor.htm',
'det_by_minor.htm',
'det_33.htm',
'det_grad_33.htm',
'mat_sum_sq.htm',
'ode_evaluate.htm',
'sparse_jac_fun.htm',
'sparse_hes_fun.htm',
'uniform_01.htm'
];
var list_down0 = [
'sparse_jac_fun.cpp.htm',
'sparse_jac_fun.hpp.htm'
];
var list_current0 = [
'sparse_jac_fun.htm#Syntax',
'sparse_jac_fun.htm#Purpose',
'sparse_jac_fun.htm#Inclusion',
'sparse_jac_fun.htm#Float',
'sparse_jac_fun.htm#FloatVector',
'sparse_jac_fun.htm#n',
'sparse_jac_fun.htm#m',
'sparse_jac_fun.htm#x',
'sparse_jac_fun.htm#row',
'sparse_jac_fun.htm#col',
'sparse_jac_fun.htm#p',
'sparse_jac_fun.htm#fp',
'sparse_jac_fun.htm#fp.Function',
'sparse_jac_fun.htm#fp.Jacobian',
'sparse_jac_fun.htm#Example',
'sparse_jac_fun.htm#Source Code'
];
function choose_across0(item)
{	var index          = item.selectedIndex;
	item.selectedIndex = 0;
	if(index > 0)
		document.location = list_across0[index-1];
}
function choose_up0(item)
{	var index          = item.selectedIndex;
	item.selectedIndex = 0;
	if(index > 0)
		document.location = list_up0[index-1];
}
function choose_down3(item)
{	var index          = item.selectedIndex;
	item.selectedIndex = 0;
	if(index > 0)
		document.location = list_down3[index-1];
}
function choose_down2(item)
{	var index          = item.selectedIndex;
	item.selectedIndex = 0;
	if(index > 0)
		document.location = list_down2[index-1];
}
function choose_down1(item)
{	var index          = item.selectedIndex;
	item.selectedIndex = 0;
	if(index > 0)
		document.location = list_down1[index-1];
}
function choose_down0(item)
{	var index          = item.selectedIndex;
	item.selectedIndex = 0;
	if(index > 0)
		document.location = list_down0[index-1];
}
function choose_current0(item)
{	var index          = item.selectedIndex;
	item.selectedIndex = 0;
	if(index > 0)
		document.location = list_current0[index-1];
}

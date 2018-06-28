var list_across0 = [
'_contents.htm',
'_reference.htm',
'_index.htm',
'_search.htm',
'_external.htm'
];
var list_up0 = [
'cppad.htm',
'appendix.htm',
'theory.htm',
'forwardtheory.htm'
];
var list_down3 = [
'install.htm',
'introduction.htm',
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
'faq.htm',
'directory.htm',
'theory.htm',
'glossary.htm',
'bib.htm',
'wish_list.htm',
'whats_new.htm',
'deprecated.htm',
'compare_c.htm',
'numeric_ad.htm',
'addon.htm',
'license.htm'
];
var list_down1 = [
'forwardtheory.htm',
'reversetheory.htm',
'reverse_identity.htm'
];
var list_down0 = [
'exp_forward.htm',
'log_forward.htm',
'sqrt_forward.htm',
'sin_cos_forward.htm',
'atan_forward.htm',
'asin_forward.htm',
'acos_forward.htm',
'tan_forward.htm',
'erf_forward.htm'
];
var list_current0 = [
'forwardtheory.htm#Taylor Notation',
'forwardtheory.htm#Binary Operators',
'forwardtheory.htm#Binary Operators.Addition',
'forwardtheory.htm#Binary Operators.Subtraction',
'forwardtheory.htm#Binary Operators.Multiplication',
'forwardtheory.htm#Binary Operators.Division',
'forwardtheory.htm#Standard Math Functions',
'forwardtheory.htm#Standard Math Functions.Differential Equation',
'forwardtheory.htm#Standard Math Functions.Taylor Coefficients Recursion Formula',
'forwardtheory.htm#Standard Math Functions.Cases that Apply Recursion Above',
'forwardtheory.htm#Standard Math Functions.Special Cases'
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
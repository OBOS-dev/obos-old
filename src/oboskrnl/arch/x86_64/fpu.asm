[BITS 64]

global fpuInit
fpuInit:
	fninit
	ret
global _fxsave
_fxsave:
	fxsave [rdi]
	ret
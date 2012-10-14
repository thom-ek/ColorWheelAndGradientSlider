VERSION		EQU	43
REVISION	EQU	2
DATE	MACRO
		dc.b	'9.3.98'
	ENDM
VERS	MACRO
		dc.b	'colorwheel.gadget 43.2'
	ENDM
VSTRING	MACRO
		dc.b	'colorwheel.gadget 43.2 (9.3.98)',13,10,0
	ENDM
VERSTAG	MACRO
		dc.b	0,'$VER: colorwheel.gadget 43.2 (9.3.98)',0
	ENDM

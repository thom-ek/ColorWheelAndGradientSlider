VERSION		EQU	43
REVISION	EQU	1
DATE	MACRO
		dc.b	'6.9.98'
	ENDM
VERS	MACRO
		dc.b	'gradientslider.gadget 43.1'
	ENDM
VSTRING	MACRO
		dc.b	'gradientslider.gadget 43.1 (6.9.98)',13,10,0
	ENDM
VERSTAG	MACRO
		dc.b	0,'$VER: gradientslider.gadget 43.1 (6.9.98)',0
	ENDM

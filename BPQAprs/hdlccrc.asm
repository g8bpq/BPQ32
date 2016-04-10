	PAGE    56,132
;
.386
.LALL
.LFCOND


CR	EQU	0DH
LF	EQU	0AH


;
;  SEGMENT definitions and order
;


;*	32 Bit code
_TEXT		SEGMENT DWORD USE32 PUBLIC 'CODE'
_TEXT		ENDS


;*	Contains 32 Bit data
_BPQDATA		SEGMENT DWORD PUBLIC 'DATA'
_BPQDATA		ENDS


	ASSUME CS:FLAT, DS:FLAT, ES:FLAT, SS:FLAT


OFFSET32 EQU <OFFSET FLAT:>

	PUBLIC	_CRCTAB

_BPQDATA		SEGMENT

_CRCTAB	DW	0,1189H,2312H,329BH,4624H
	DW	57ADH,6536H,74BFH,8C48H,9DC1H
	DW	0AF5AH,0BED3H,0CA6CH,0DBE5H,0E97EH	
	DW	0F8F7H,1081H,0108H,3393H,221AH
	DW	56A5H,472CH,75B7H,643EH,9CC9H	
	DW	8D40H,0BFDBH,0AE52H,0DAEDH,0CB64H
	DW	0F9FFH,0E876H,2102H,308BH,210H	
	DW	1399H,6726H,76AFH,4434H,55BDH	
	DW	0AD4AH,0BCC3H,8E58H,9FD1H,0EB6EH	
	DW	0FAE7H,0C87CH,0D9F5H,3183H,200AH	
	DW	1291H,318H,77A7H,662EH,54B5H
	DW	453CH,0BDCBH,0AC42H,9ED9H,8F50H
	DW	0FBEFH,0EA66H,0D8FDH,0C974H,4204H	
	DW	538DH,6116H,709FH,420H,15A9H
	DW	2732H,36BBH,0CE4CH,0DFC5H,0ED5EH	
	DW	0FCD7H,8868H
	DW	99E1H,0AB7AH,0BAF3H,5285H,430CH	
	DW	7197H,601EH,14A1H,528H,37B3H
	DW	263AH,0DECDH,0CF44H,0FDDFH,0EC56H	
	DW	98E9H,8960H,0BBFBH,0AA72H,6306H
	DW	728FH,4014H,519DH,2522H,34ABH
	DW	630H,17B9H,0EF4EH,0FEC7H,0CC5CH
	DW	0DDD5H,0A96AH,0B8E3H,8A78H,9BF1H	
	DW	7387H,620EH,5095H,411CH,35A3H	
	DW	242AH,16B1H,738H,0FFCFH,0EE46H	
	DW	0DCDDH,0CD54H,0B9EBH,0A862H,9AF9H	
	DW	8B70H,8408H,9581H,0A71AH,0B693H	
	DW	0C22CH,0D3A5H,0E13EH,0F0B7H,840H	
	DW	19C9H,2B52H,3ADBH,4E64H,5FEDH	
	DW	6D76H,7CFFH,9489H,8500H,0B79BH	
	DW	0A612H,0D2ADH,0C324H,0F1BFH,0E036H	
	DW	18C1H,948H,3BD3H,2A5AH,5EE5H	
	DW	4F6CH,7DF7H,6C7EH,0A50AH,0B483H	
	DW	8618H,9791H,0E32EH,0F2A7H,0C03CH	
	DW	0D1B5H,2942H,38CBH,0A50H,1BD9H	
	DW	6F66H,7EEFH,4C74H,5DFDH,0B58BH	
	DW	0A402H,9699H,8710H,0F3AFH,0E226H	
	DW	0D0BDH,0C134H,39C3H,284AH,1AD1H	
	DW	0B58H,7FE7H,6E6EH,5CF5H,4D7CH
	DW	0C60CH,0D785H,0E51EH,0F497H,8028H	
	DW	91A1H,0A33AH,0B2B3H,4A44H,5BCDH	
	DW	6956H,78DFH,0C60H,1DE9H,2F72H	
	DW	3EFBH,0D68DH,0C704H,0F59FH,0E416H	
	DW	90A9H,8120H,0B3BBH,0A232H,5AC5H	
	DW	4B4CH,79D7H,685EH,1CE1H,0D68H	
	DW	3FF3H,2E7AH,0E70EH,0F687H,0C41CH	
	DW	0D595H,0A12AH,0B0A3H,8238H,93B1H	
	DW	6B46H,7ACFH,4854H,59DDH,2D62H	
	DW	03CEBH,0E70H,1FF9H,0F78FH,0E606H	
	DW	0D49DH,0C514H,0B1ABH,0A022H,92B9H	
	DW	8330H,7BC7H,6A4EH,58D5H,495CH	
	DW	3DE3H,2C6AH,1EF1H,0F78H		

_BPQDATA	ENDS

;_TEXT	SEGMENT

;_CALC_CRC:
;


;	XOR	DL,AL		; OLD FCS .XOR. CHAR
;	MOVZX EBX,DL		; CALC INDEX INTO TABLE FROM BOTTOM 8 BITS
;	ADD	EBX,EBX
;	MOV	DL,DH		; SHIFT DOWN 8 BITS
;	XOR	DH,DH		; AND CLEAR TOP BITS
;	XOR	DX,[EBX+CRCTAB]	; XOR WITH TABLE ENTRY
;	RET
               

;_TEXT	ENDS

	END


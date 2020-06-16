dmess140:	db	clrlcd, curblink
		db	'Schroefsnelheden in % per spaakgroep :  '
		db	'     boven 1 : ---       boven 2 : ---  '
		db	'     onder 1 : ---       onder 2 : ---  '
		db	'druk MENU voor doorgaan ...           23', eos
emess140:	db	clrlcd, curblink
		db	'Screwspeeds in % (0..100) per spokegroup'
		db	'    top    1 : ---      top    2 : ---  '
		db	'    bottom 1 : ---      bottom 2 : ---  '
		db	'press MENU to continue                23', eos
fmess140:	db	clrlcd, curblink
		db	'Vitesse tournevis en % (0.100) par nappe'
		db	'    haute  1 : ---      haute  2 : ---  '
		db	'    bas    1 : ---      bas    2 : ---  '
		db	'appuyez MENU pour continuer ...       23', eos
gmess140:	db	clrlcd, curblink
		db	'Drehzahl in % (0.100) pro Speichengruppe'
		db	'     oben  1 : ---       oben  2 : ---  '
		db	'     unten 1 : ---       unten 2 : ---  '
		db	'druecken Sie MENU zum weitergehen ... 23', eos
imess140:	db	clrlcd, curblink
		db	'velocita cacciavite in % per gruppo     '
		db	'     alto  1 : ---       alto  2 : ---  '
		db	'     basso 1 : ---       basso 2 : ---  '
		db	'spingere MENU per continuare ...      23', eos

dmess220:	db	clrlcd, curoff
		db	'schroefsnelheid :   --- %               '
		db	'gemeten snelheid: ----- omw per min     '
		db	'                                        '
		db	'druk MENU voor doorgaan ...           34', eos
emess220:	db	clrlcd, curoff
		db	'set speed       :   --- %               '
		db	'measured speed  : ----- rpm             '
		db	'                                        '
		db	'press MENU to continue ...            34', eos
fmess220:	db	clrlcd, curoff
		db	'Vitesse tournev.:   --- %               '
		db	'Controle vitesse: ----- tpm             '
		db	'                                        '
		db	'appuyez MENU pour continuer ...       34', eos
gmess220:	db	clrlcd, curoff
		db	'Geschw. Schrz.  :   --- %               '
		db	'gemessen        : ----- upm             '
		db	'                                        '
		db	'druecken Sie MENU zum weitergehen ... 34', eos
imess220:	db	clrlcd, curoff
		db	'Velocita cacciav:   --- %               '
		db	'Controlo velocit: ----- upm             '
		db	'                                        '
		db	'spingere MENU per continuare ...      34', eos

touchthecow:	db	'%00001111'

fixthisline:	db	%00001111

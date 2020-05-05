(symbol? 'foo)
(symbol? (car '(a b)))
(symbol? "bar")
(symbol? 'nil)
(symbol? '()) 
(symbol? #f)
(symbol=? 'a 'a)
(symbol=? 'a 'A) 
; XXX our symbol=? isn't variadic or has compiler support yet
; XXX neither is string=?
;(symbol=? 'a 'a 'a)
;(symbol=? 'a 'a 'A)
(symbol->string 'flying-fish)
(symbol->string 'Martin)
(symbol->string (string->symbol "Malvina"))
(string->symbol "mISSISSIppi")
(eq? 'bitBlt (string->symbol "bitBlt"))
(eq? 'LollyPop (string->symbol (symbol->string 'LollyPop)))
(string=? "K. Harper, M.D." (symbol->string (string->symbol "K. Harper, M.D.")))
===
#t
#t
#f
#t
#f
#f
#t
#f
"flying-fish"
"Martin"
"Malvina"
mISSISSIppi
#t
#t
#t

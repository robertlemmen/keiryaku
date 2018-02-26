; comments
'atom
(quote atom)
'turkey
1492
'*abc$
(quote *abc$)
'(atom)
(quote (atom))
'(atom turkey or)
'((atom turkey) or)
'xyz
'(x y z)
'((x y z))
'(how are you doing so far)
'(((how) are) ((you) (doing so)) far)
'()
'(() () () ())
'(atom turkey) 'or
'()     ; because it's a list
(car '(a b c))                ; 'a
(car '((a b c) x y z))        ; '(a b c)
(car '(((hotdogs)) (and) (pickle) relish))  ; '((hotdogs))
(car (car '(((hotdogs)) (and))))            ; '(hotdogs)
(cdr '(a b c))              ; '(b c)
(cdr '((a b c) x y z))      ; '(x y z)
(cdr '(hamburger))          ; '()
(cdr '((x) t r))            ; '(t r)
(car (cdr '((b) (x y) ((c)))))      ; '(x y)
(cdr (cdr '((b) (x y) ((c)))))      ; '(((c)))
(cons 'peanut '(butter and jelly))                  ; '(peanut butter and jelly)
(cons '(banana and) '(peanut butter and jelly))     ; '((banana and) peanut butter and jelly)
(cons '((help) this) '(is very ((hard) to learn)))  ; '(((help) this) is very ((hard) to learn))
(cons '(a b (c)) '())                               ; '((a b (c)))
(cons 'a '())                                       ; '(a)
(cons 'a (car '((b) c d)))     ; (a b)
(cons 'a (cdr '((b) c d)))     ; (a c d)
'()
(null? '())         ; true
(null? '(a b c))    ; false
(define atom?
 (lambda (x)
    (and (not (pair? x)) (not (null? x)))))
(atom? 'Harry)                          ; true
(atom? '(Harry had a heap of apples))   ; false
(atom? (car '(Harry had a heap of apples)))         ; true
(atom? (cdr '(Harry had a heap of apples)))         ; false
(atom? (cdr '(Harry)))                              ; false
(atom? (car (cdr '(swing low sweet cherry oat))))   ; true
(atom? (car (cdr '(swing (low sweet) cherry oat)))) ; false
(eq? 'Harry 'Harry)         ; true
(eq? 'margarine 'butter)    ; false
(eq? (car '(Mary had a little lamb chop)) 'Mary)        ; true
(eq? (car '(beans beans)) (car (cdr '(beans beans))))   ; true
(define member*
    (lambda (a l)
        (cond
            ((null? l) 
                #f)
            ((atom? (car l))
                (or (eq? (car l) a)
                    (member* a (cdr l))))
            (else
                (or (member* a (car l))
                    (member* a (cdr l)))))))

(member*
  'chips
  '((potato) (chips ((with) fish) (chips))))
===
atom
atom
turkey
1492
*abc$
*abc$
(atom)
(atom)
(atom turkey or)
((atom turkey) or)
xyz
(x y z)
((x y z))
(how are you doing so far)
(((how) are) ((you) (doing so)) far)
()
(() () () ())
(atom turkey)
or
()
a
(a b c)
((hotdogs))
(hotdogs)
(b c)
(x y z)
()
(t r)
(x y)
(((c)))
(peanut butter and jelly)
((banana and) peanut butter and jelly)
(((help) this) is very ((hard) to learn))
((a b (c)))
(a)
(a b)
(a c d)
()
#t
#f
#t
#f
#t
#f
#f
#t
#f
#t
#f
#t
#t
#t

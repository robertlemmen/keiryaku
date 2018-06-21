; pre-compiler base env
(define even?
    (lambda (arg)
        (eq? (* (/ arg 2) 2) arg)))

(define zero?
    (lambda (n)
        (eq? n 0)))

(define caar
    (lambda (arg)
        (car (car arg))))

(define cadr
    (lambda (arg)
        (car (cdr arg))))

(define cdar
    (lambda (arg)
        (cdr (car arg))))

(define cddr
    (lambda (arg)
        (cdr (cdr arg))))

(define caaar
    (lambda (arg)
        (car (car (car arg)))))

(define caadr
    (lambda (arg)
        (car (car (cdr arg)))))

(define cadar
    (lambda (arg)
        (car (cdr (car arg)))))

(define caddr
    (lambda (arg)
        (car (cdr (cdr arg)))))

(define cdaar
    (lambda (arg)
        (cdr (car (car arg)))))

(define cdadr
    (lambda (arg)
        (cdr (car (cdr arg)))))

(define cddar
    (lambda (arg)
        (cdr (cdr (car arg)))))

(define cdddr
    (lambda (arg)
        (cdr (cdr (cdr arg)))))

(define caaaar
    (lambda (arg)
        (car (car (car (car arg))))))

(define caaadr
    (lambda (arg)
        (car (car (car (cdr arg))))))

(define caadar
    (lambda (arg)
        (car (car (cdr (car arg))))))

(define caaddr
    (lambda (arg)
        (car (car (cdr (cdr arg))))))

(define cadaar
    (lambda (arg)
        (car (cdr (car (car arg))))))

(define cadadr
    (lambda (arg)
        (car (cdr (car (cdr arg))))))

(define caddar
    (lambda (arg)
        (car (cdr (cdr (car arg))))))

(define cadddr
    (lambda (arg)
        (car (cdr (cdr (cdr arg))))))

(define cdaaar
    (lambda (arg)
        (cdr (car (car (car arg))))))

(define cdaadr
    (lambda (arg)
        (cdr (car (car (cdr arg))))))

(define cdadar
    (lambda (arg)
        (cdr (car (cdr (car arg))))))

(define cdaddr
    (lambda (arg)
        (cdr (car (cdr (cdr arg))))))

(define cddaar
    (lambda (arg)
        (cdr (cdr (car (car arg))))))

(define cddadr
    (lambda (arg)
        (cdr (cdr (car (cdr arg))))))

(define cdddar
    (lambda (arg)
        (cdr (cdr (cdr (car arg))))))

(define cddddr
    (lambda (arg)
        (cdr (cdr (cdr (cdr arg))))))

(define _emit-cond-body
    (lambda (ex next _compile)
        (if (cdr ex) 
            (if (eq? (cadr ex) '=>)
; XXX this really needs a let in the resulting code rather than running (car ex)
; twice. also probably doesn't work for an inline-defined lambda, another let
                (list 'if (_compile (car ex)) (list (_compile (caddr ex)) (_compile (car ex))) (_emit-cond-case next _compile)) 
                (list 'if (_compile (car ex)) (list 'begin (_compile (cadr ex))) (_emit-cond-case next _compile)) )
            (list 'if (_compile (car ex)) (list 'begin (_compile (cadr ex))) (_emit-cond-case next _compile)) )))

(define _emit-cond-case
    (lambda (ex _compile) 
        (if (null? ex)
            '()
            (if (eq? (caar ex) 'else)
                (_compile (cadar ex))
                (_emit-cond-body (car ex) (cdr ex) _compile))) ))


; XXX too many lambdas to support case, fold them together
(define _emit-case-body
    (lambda (ex _compile)
        (if (eq? (car ex) '=>)
            (list 'let (list (list 'func (cadr ex))) (list 'func 'result))
            (car ex) )))

(define _emit-case-entry
    (lambda (ex _compile)
        (if (null? (cdr ex))
            (if (eq? (caar ex) 'else)
                (_emit-case-body (cdar ex) _compile)
                (list 'if (list 'memv 'result (list 'quote (caar ex))) (cadar ex) #f))
            (list 'if (list 'memv 'result (list 'quote (caar ex))) (cadar ex) (_emit-case-entry (cdr ex) _compile)) )))

(define _emit-case-case
    (lambda (ex _compile)
        (list 'let (list (list 'result (_compile (car ex)))) (_emit-case-entry (cdr ex) _compile)) ))


(define _emit-and-case
    (lambda (ex _compile)
        (if (pair? ex)
            (if (null? (cdr ex))
; XXX I bet the let needs to be in the emitted syntax, not to call this twice.
; same for 'or' below
                (let [(cex (_compile (car ex)))]
                    (list 'if cex cex #f))
                (list 'if (_compile (car ex)) (_emit-and-case (cdr ex) _compile) #f))
            ex)))

(define _emit-or-case
    (lambda (ex _compile)
        (if (pair? ex)
            (if (null? (cdr ex))
                (let [(cex (_compile (car ex)))]
                    (list 'if cex cex #f))
                (list 'if (_compile (car ex)) #t (_emit-and-case (cdr ex) _compile)))
            ex)))

(define sub1
    (lambda (n)
        (- n 1)))

(define add1
    (lambda (n)
        (+ n 1)))
        
(define _compile
    (let [
        (compile-and
            (lambda (ex _compile)
                (if (null? ex)
                    #t
                    (_emit-and-case ex _compile))))
        (compile-or
            (lambda (ex _compile)
                (if (null? ex)
                    #f
                    (_emit-or-case ex _compile))))
        (compile-not
            (lambda (ex _compile)
                (list 'if (_compile (car ex)) #f #t) ))
        (compile-cond
            (lambda (ex _compile)
; XXX darn seems we need the recursive let version, then we can avoid the extra
; define above
                (_emit-cond-case ex _compile)) )
        (compile-case
            (lambda (ex _compile)
; XXX darn seems we need the recursive let version, then we can avoid the extra
; define above
                (_emit-case-case ex _compile)) )
        (compile-make-vector
            (lambda (ex _compile)
                (if (null? (cdr ex))
                    (cons 'make-vector (cons (car ex) (list 0)))
                    (cons 'make-vector ex))))
        (compile-let
            (lambda (ex _compile)
                (if (null? (cddr ex))
                    (cons 'let ex)
                    (list 'let (car ex) (cons 'begin (cdr ex))))))
        (compile-arity2-member
            (lambda (ex _compile)
                (if (null? (cddr ex))
                    (list '_memberg (car ex) (cadr ex) 'equal?)
                    (cons '_memberg ex))))
        ]
        (lambda (ex)
            (if (pair? ex)
                (if (eq? (car ex) 'and)
                    (compile-and (cdr ex) _compile)
                    (if (eq? (car ex) 'or)
                        (compile-or (cdr ex) _compile)
                        (if (eq? (car ex) 'not)
                            (compile-not (cdr ex) _compile)
                            (if (eq? (car ex) 'cond)
                                (compile-cond (cdr ex) _compile)
                                (if (eq? (car ex) 'case)
                                    (compile-case (cdr ex) _compile)
                                    (if (eq? (car ex) 'let)
                                        (compile-let (cdr ex) _compile)
                                        (if (eq? (car ex) 'member)
                                            (compile-arity2-member (cdr ex) _compile)
                                            (if (eq? (car ex) 'quote)
                                                ex
                                                (if (eq? (car ex) 'make-vector)
                                                    (compile-make-vector (cdr ex) _compile)
                                                    (cons (_compile (car ex)) (_compile (cdr ex))))))))))))
                ex))))

; some base environment, should probably be separate from compiler, and should
; use compiler
(define list
    (lambda args
        args))

(define length
    (lambda (l)
        (if (null? l)
            0
            (+ 1 (length (cdr l))))))

(define vector
    (let [(list-vec-copy-rec (lambda (l v i)
        (if (pair? l)
            (begin
                (vector-set! v i (car l))
                (list-vec-copy-rec (cdr l) v (+ 1 i))
            )
            '())))]
    (lambda vals
        (let [(rv (make-vector (length vals) 0))]
            (begin
                (list-vec-copy-rec vals rv 0)
                rv)))))

; XXX the assoc with compare is the same as this...
(define _assg
    (lambda (obj alist comp)
        (if (null? alist)
            #f
            (if (comp obj (caar alist))
                (car alist)
                (assq obj (cdr alist))))))

(define assq
    (lambda (obj alist)
        (_assg obj alist eq?)))

(define assv
    (lambda (obj alist)
        (_assg obj alist eqv?)))

; XXX assoc needs "compare" case
(define assoc
    (lambda (obj alist)
        (_assg obj alist equal?)))

(define _memberg
    (lambda (obj list compare)
        (if (null? list)
            #f
            (if (compare obj (car list))
                list
                (member obj (cdr list) compare))) ))

(define memq
    (lambda (obj list)
        (_memberg obj list eq?) ))

(define memv
    (lambda (obj list)
        (_memberg obj list eqv?) ))

(define boolean=? 
    (lambda (a b . m)
; XXX does not test that they are booleans at the moment
        (if (and (pair? m) (not (null? (car m))))
            (if (or (and a b) (and (not a) (not b)))
                (apply boolean=? (cons b (cons (car m) (cdr m))))
                #f)
            (or (and a b) (and (not a) (not b))))))

; some base environment, should probably be separate from compiler
(define list
    (lambda args
        args))

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

(define _emit-cond-case
    (lambda (ex _compile) 
        (if (eq? (caar ex) 'else)
            (_compile (cadar ex))
            (list 'if (_compile (caar ex)) (list 'begin (_compile (cadar ex))) (_emit-cond-case (cdr ex) _compile)) )))
        
(define _compile
    (let [
        (compile-and
            (lambda (ex _compile)
                (list 'if (_compile (car ex))
                        (list 'if (_compile (cadr ex)) 
                                #t
                                #f)
                        #f) ))
        (compile-or
            (lambda (ex _compile)
                (list 'if (_compile (car ex)) 
                        #t
                        (list 'if (_compile (cadr ex)) #t #f) )))
        (compile-not
            (lambda (ex _compile)
                (list 'if (_compile (car ex)) #f #t) ))
        (compile-cond
            (lambda (ex _compile)
; XXX darn seems we need the recursive let version, then we can avoid the extra
; define above
                (_emit-cond-case ex _compile)) )
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
                                (if (eq? (car ex) 'quote)
                                    ex
                                    (cons (_compile (car ex)) (_compile (cdr ex))))))))
                ex))))

; this is silly of course, but shows that the compilation tower somewhat works
;(define _compile
;    (lambda (ex)
;        (cons '+ (cons 1 (cons ex '())))))

(define _compile
    (let [
        (compile-and
            (lambda (ex _compile)
                (cons 'if 
                    (cons (_compile (car ex)) 
                        (cons (cons 'if 
                            (cons (_compile (car (cdr ex))) 
                                (cons #t 
                                (cons #f '())))) 
                            (cons #f '()))))))
        (compile-or
            (lambda (ex _compile)
                (cons 'if 
                    (cons (_compile (car ex)) 
                        (cons #t 
                        (cons (cons 'if (cons (_compile (car (cdr ex))) (cons #t (cons #f '())))) '())
                        )))))
        (compile-not
            (lambda (ex _compile)
                (cons 'if 
                    (cons (_compile (car ex)) 
                        (cons #f (cons #t '()))))))
        ]
        (lambda (ex)
            (if (pair? ex)
                (if (eq? (car ex) 'and)
                    (compile-and (cdr ex) _compile)
                    (if (eq? (car ex) 'or)
                        (compile-or (cdr ex) _compile)
                        (if (eq? (car ex) 'not)
                            (compile-not (cdr ex) _compile)
                            (if (eq? (car ex) 'quote)
                                ex
                                (cons (_compile (car ex)) (_compile (cdr ex)))))))
                ex))))

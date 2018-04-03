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

(define _emit-cond-case
    (lambda (ex _compile) 
        (if (null? ex)
            '()
            (if (eq? (caar ex) 'else)
                (_compile (cadar ex))
                (list 'if (_compile (caar ex)) (list 'begin (_compile (cadar ex))) (_emit-cond-case (cdr ex) _compile)) ))))

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
        (compile-make-vector
            (lambda (ex _compile)
                (if (null? (cdr ex))
                    (cons 'make-vector (cons (car ex) (list 0)))
                    (cons 'make-vector ex))))
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
                                    (if (eq? (car ex) 'make-vector)
                                        (compile-make-vector (cdr ex) _compile)
                                        (cons (_compile (car ex)) (_compile (cdr ex)))))))))
                ex))))

; some base environment, should probably be separate from compiler, and should
; use compiler
(define list
    (lambda args
        args))

(define length
    (let [(length-rec (lambda vals
            (if (pair? vals)
                (+ 1 (apply length-rec (cdr vals)))
                0)))]
        (lambda (l)
            (apply length-rec l))))

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


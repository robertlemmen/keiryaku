;; these are cases from "The little Schemer", taken and adapted from 
;; https://github.com/pkrumins/the-little-schemer.
(define atom?
 (lambda (x)
    (and (not (pair? x)) (not (null? x)))))
(define numbered?
  (lambda (aexp)
    (cond
      ((atom? aexp) (number? aexp))
      ((eq? (car (cdr aexp)) 'o+)
       (and (numbered? (car aexp))
            (numbered? (car (cdr (cdr aexp))))))
      ((eq? (car (cdr aexp)) 'ox)
       (and (numbered? (car aexp))
            (numbered? (car (cdr (cdr aexp))))))
      ((eq? (car (cdr aexp)) 'o^)
       (and (numbered? (car aexp))
            (numbered? (car (cdr (cdr aexp))))))
      (else #f))))
(numbered? '5)
(numbered? '(5 o+ 5))
(numbered? '(5 o+ a))
(numbered? '(5 ox (3 o^ 2)))
(numbered? '(5 ox (3 'foo 2)))
(numbered? '((5 o+ 2) ox (3 o^ 2)))
(define numbered?
  (lambda (aexp)
    (cond
      ((atom? aexp) (number? aexp))
      (else
        (and (numbered? (car aexp))
             (numbered? (car (cdr (cdr aexp)))))))))
(numbered? '5)
(numbered? '(5 o+ 5))
(numbered? '(5 ox (3 o^ 2)))
(numbered? '((5 o+ 2) ox (3 o^ 2)))
(define value
  (lambda (nexp)
    (cond
      ((atom? nexp) nexp)
      ((eq? (car (cdr nexp)) 'o+)
       (+ (value (car nexp))
          (value (car (cdr (cdr nexp))))))
      ((eq? (car (cdr nexp)) 'o*)
       (* (value (car nexp))
          (value (car (cdr (cdr nexp))))))
      ((eq? (car (cdr nexp)) 'o^)
       (expt (value (car nexp))
             (value (car (cdr (cdr nexp))))))
      (else #f))))
(value 13)
(value '(1 o+ 3))
(value '(1 o+ (3 o^ 4)))
(define value-prefix
  (lambda (nexp)
    (cond
      ((atom? nexp) nexp)
      ((eq? (car nexp) 'o+)
       (+ (value-prefix (car (cdr nexp)))
          (value-prefix (car (cdr (cdr nexp))))))
      ((eq? (car nexp) 'o*)
       (* (value-prefix (car (cdr nexp)))
          (value-prefix (car (cdr (cdr nexp))))))
      ((eq? (car nexp) 'o^)
       (expt (value-prefix (car (cdr nexp)))
             (value-prefix (car (cdr (cdr nexp))))))
      (else #f))))
(value-prefix 13)
(value-prefix '(o+ 3 4))
(value-prefix '(o+ 1 (o^ 3 4)))
(define 1st-sub-exp
  (lambda (aexp)
    (car (cdr aexp))))
(define 2nd-sub-exp
  (lambda (aexp)
    (car (cdr (cdr aexp)))))
(define operator
  (lambda (aexp)
    (car aexp)))
(define value-prefix-helper
  (lambda (nexp)
    (cond
      ((atom? nexp) nexp)
      ((eq? (operator nexp) 'o+)
       (+ (value-prefix (1st-sub-exp nexp))
          (value-prefix (2nd-sub-exp nexp))))
      ((eq? (car nexp) 'o*)
       (* (value-prefix (1st-sub-exp nexp))
          (value-prefix (2nd-sub-exp nexp))))
      ((eq? (car nexp) 'o^)
       (expt (value-prefix (1st-sub-exp nexp))
             (value-prefix (2nd-sub-exp nexp))))
      (else #f))))
(value-prefix-helper 13)
(value-prefix-helper '(o+ 3 4))
(value-prefix-helper '(o+ 1 (o^ 3 4)))
(define 1st-sub-exp
  (lambda (aexp)
    (car aexp)))
(define 2nd-sub-exp
  (lambda (aexp)
    (car (cdr (cdr aexp)))))
(define operator
  (lambda (aexp)
    (car (cdr aexp))))
(value-prefix 13)
(value-prefix '(o+ 3 4))
(value-prefix '(o+ 1 (o^ 3 4)))
(define sero?
  (lambda (n)
    (null? n)))
(define edd1
  (lambda (n)
    (cons '() n)))
(define zub1
  (lambda (n)
    (cdr n)))
; XXX the original used ".+" as the identifier, which our parser can't handle
; properly yet
(define a+
  (lambda (n m)
    (cond
      ((sero? m) n)
      (else
        (edd1 (a+ n (zub1 m)))))))
(a+ '(()) '(() ()))
(define tat?
  (lambda (l)
    (cond
      ((null? l) #t)
      ((atom? (car l))
       (tat? (cdr l)))
      (else #f))))
(tat? '((()) (()()) (()()())))
===
#t
#t
#f
#t
#f
#t
#t
#t
#t
#t
13
4
82
13
7
82
13
7
82
13
7
82
(() () ())
#f


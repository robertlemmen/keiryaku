(define pa1 (make-parameter 1 (lambda (x) x)))
(pa1) ; should be 1
(parameterize ((pa1 2))
  (list
    (pa1) ; should be 2
    (parameterize ((pa1 3))
      (pa1))
    (pa1) ; should be 2
  ))
(pa1) ; should be 1 again
(procedure? pa1) ; should be #t

(define pa2 (make-parameter 11 (lambda (x) x)))
(pa2) 
(parameterize ((pa2 12))
  (list
    (pa2)
    (parameterize ((pa2 13))
      (pa2))
    (pa2)
  ))
(pa2)
(define pa3 (make-parameter 21 (lambda (x) (+ 20 x))))
(pa3) 
(parameterize ((pa3 2))
  (list
    (pa3)
    (parameterize ((pa3 3))
      (pa3))
    (pa3)
  ))
(pa3)
===
1
(2 3 2)
1
#t
11
(12 13 12)
11
21
(22 23 22)
21

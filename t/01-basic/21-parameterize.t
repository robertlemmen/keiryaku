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
===
1
(2 3 2)
1
#t

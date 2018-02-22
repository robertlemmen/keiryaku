((lambda (n)
    (if (= n 0)
        0
        (+ n 1)))
 8)
(define factorial
    (lambda (n)
        (if (= 0 n)
            1
            (* n (factorial (- n 1))))))

(factorial 6)

(define sum-list
      (lambda (vals)
          (if (pair? vals)
              (+ (car vals) (sum-list (cdr vals)))
              0)))
(define sum
    (lambda vals
        (sum-list vals)))

(sum-list '(1 2 3))
(sum 1 2 3)

(define sum2
    (lambda vals
          (if (pair? vals)
              (+ (car vals) (apply sum2 (cdr vals)))
              0)))

(apply sum2 '(1 3 5))
(list 'x 1 2)
(list 'y (list 'x 1 2) 3)
===
9
720
6
6
9
(x 1 2)
(y (x 1 2) 3)

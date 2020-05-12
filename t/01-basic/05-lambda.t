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

(define test1 (lambda (a b) (list a b)))
(test1 1 2)

(define test2 (lambda vals vals))
(test2 )
(test2 'a)
(test2 'a 'b)

(define test3 (lambda (x y . z) (list x y z)))
(test3 3 4)
(test3 3 4 5)
(test3 3 4 5 6)
(apply + '())
(apply + 4 '())
(apply + '(5))
(apply + '(1 2))
(apply + '(1 2 3))
(apply + 4 '(1 2 3))
(apply + 4 5 '(1 2) )
(define mything
    (lambda (op . list)
        (apply op list)))

(apply mything + '(1 2 3))
(define count
    (lambda (list)
        (if (null? list)
            0
            (+ 1 (count (cdr list))))))
(apply + '(100 11))
(apply + '(4 5 6))
(apply + '(5 6))
(apply count '((4 5 6)))
(apply count '((list 4 5 6)))
(define xt
    (lambda (x)
        (+ 1 2)
        (+ 2 x)))
(xt 5)
(procedure? car)
(procedure? (lambda (x) (+ x x)))
(procedure? 'car)
(define test (lambda () 23))
(test)
===
9
720
6
6
9
(x 1 2)
(y (x 1 2) 3)
(1 2)
()
(a)
(a b)
(3 4 ())
(3 4 (5))
(3 4 (5 6))
0
4
5
3
6
10
12
6
111
15
11
3
4
7
#t
#t
#f
23

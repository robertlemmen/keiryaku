(define number->list 
    (lambda (m)
        (let loop ((n m) (acc '()))
            (if (< n 10)
                (cons n acc)
                (loop (quotient n 10)
                    (cons (remainder n 10) acc))))))

(number->list 2016)
===
(2 0 1 6)

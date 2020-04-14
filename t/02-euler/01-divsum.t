; Problem 1

; If we list all the natural numbers below 10 that are multiples of 3 or 5, we
; get 3, 5, 6 and 9. The sum of these multiples is 23.
;
; Find the sum of all the multiples of 3 or 5 below 1000.

; lazyness would make this more scalable, but this is simple and clean
(let ([wanted-num? (lambda (x)
        (or (= 0 (truncate-remainder x 3)) 
            (= 0 (truncate-remainder x 5) )))] )

  (fold-left + 0
    (filter wanted-num? 
      (iota 999 1))) )
===
233168

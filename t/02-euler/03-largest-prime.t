; Problem 3: Largest prime factor

; The prime factors of 13195 are 5, 7, 13 and 29.

; What is the largest prime factor of the number 600851475143 ?

(define (factorize number)
  (let fact-int ((div 2) (num number))
    (if (> (* div div) num)
      (list num)
      (if (= (truncate-remainder num div) 0)
        (cons div (fact-int div (/ num div)))
        (fact-int (+ 1 div) num) ))))

(factorize 13195)
; needs integers > 32 bit
;(factorize 600851475143)
===
(5 7 13 29)

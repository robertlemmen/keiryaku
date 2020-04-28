; Problem 5: Smallest multiple

; 2520 is the smallest number that can be divided by each of the numbers from 1
; to 10 without any remainder.

; What is the smallest positive number that is evenly divisible by all of the
; numbers from 1 to 20?

; XXX most of these should be in our base definitions
(define (abs x) (if (< x 0) (- x) x))

(define (modulo a b)
  (let ((rem (remainder a b)))
    (if (< b 0)
        (if (<= rem 0) res (+ rem b))
        (if (>= rem 0) res (+ rem b)))))

(define (_gcd2 a b)
  (if (= b 0)
      (abs a)
      (_gcd2 b (remainder a b))))

(define (gcd . args)
  (fold-left _gcd2 1 args))

(define (_lcm2 a b)
  (abs (quotient (* a b) (_gcd2 a b))))

(define (lcm . args)
  (fold-left _lcm2 1 args))

; result for 20 is 232792560

; XXX but we would also need integers greater 32 bits to compute that...
; XXX and we need argument overflow logic to do do anything > 8
(apply lcm (iota 6 1))
===
60

;; these are taken and adapted from 
;; https://github.com/gypified/chibi-scheme/blob/master/tests/r7rs-tests.scm
;; XXX this should be converted to the "test" framework used by chibi once
;; we can actually run that (needs define-syntax et al)

; XXX incomplete section!

(cond ((> 3 2) 'greater)
        ((< 3 2) 'less))

(cond ((> 3 3) 'greater)
        ((< 3 3) 'less)
        (else 'equal))

(cond ((assv 'b '((a 1) (b 2))) => cadr)
      (else #f))

(case (* 2 3)
      ((2 3 5 7) 'prime)
      ((1 4 6 8 9) 'composite))

(case (car '(c d))
      ((a e i o u) 'vowel)
      ((w y) 'semivowel)
      (else => (lambda (x) x)))

(map (lambda (x)
        (case x
            ((a e i o u) => (lambda (w) (cons 'vowel w)))
            ((w y) (cons 'semivowel x))
            (else => (lambda (w) (cons 'other w)))))
        '(z y x w u))


(and (= 2 2) (> 2 1))
(and (= 2 2) (< 2 1))
(and 1 2 'c '(f g))
(and)
(or (= 2 2) (> 2 1))
(or (= 2 2) (< 2 1))
(or #f #f #f)
(or (memq 'b '(a b c))
    (/ 3 0))
(let ((x 2) (y 3))
  (* x y))
(let ((x 2) (y 3))
  (let ((x 7)
        (z (+ x y)))
    (* z x)))
(let ((x 2) (y 3))
  (let* ((x 7)
         (z (+ x y)))
    (* z x)))
===
greater
equal
2
composite
c
((other . z) (semivowel . y) (other . x) (semivowel . w) (vowel . u))
#t
#f
(f g)
#t
#t
#t
#f
(b c)
6
35
70

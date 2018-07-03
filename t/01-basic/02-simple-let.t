(let [(x 2) (y 3)]
    (let [(x y) (z 2)]
        (+ (+ x y) z)))
(let ((x 2) (y 3))
  (let [(x 7) (z (+ x y))]
    (* z x)))
===
8
35

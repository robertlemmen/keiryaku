(define _repl
    (lambda (fh)
        (let ((expr (read fh)))
            (if (eof-object? expr)
                '()
                (let ((result (eval (_compile expr) _top_env)))
                    (if (_nil? result)
                        '()
                        (begin
                            (display result stdout)
                            (newline stdout)
                        ))
                    (_repl fh)) ))))

(_repl stdin)

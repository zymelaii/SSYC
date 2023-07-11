use lib.nu
use lib *

# List all available testcase categories.
export def "testcase list" [] {
    let dir = (git-ws | path join 'test/testcase')
    if ($dir | path type) != 'dir' {
        throw IOError "can not find directory `testcase`" (metadata $dir)
    }
    ls $dir
        | where type == dir
        | each {|e|
            $e.name | path relative-to $dir
        }
}

# Get testcases from specific category.
export def "testcase get" [
    category: string@"testcase list", # Category of desired testcases
] {
    let dir = (git-ws | path join $'test/testcase/($category)')
    if ($dir | path type) != 'dir' {
        throw IOError $"testcase of category `($category)` is not available" (metadata $category)
    }
    ls $'($dir)/*.sy'
        | where type == file
        | get name
        | par-each {|e| $e | path relative-to $dir | parse -r '(?<id>\d+)_(?<name>\w+)\.sy$'}
        | flatten
        | par-each {|e|
            let has_sample_in = ($'($dir)/($e.id)_($e.name).in' | path type) == 'file'
            let has_sample_out = ($'($dir)/($e.id)_($e.name).out' | path type) == 'file'
            [[id name sample_in sample_out]; [$e.id $e.name $has_sample_in $has_sample_out]]
        }
        | sort-by id -n
}

# Run testcases from specific category.
export def "testcase run" [
    category: string@"testcase list",   # Category of desired testcases
    --arguments (-v): string,           # Arguments passed to the executable
    --print-failed-only (-f),           # Print failed testcase only
    --quiet (-q),                       # Print test summary only
] {
    let dir = (git-ws | path join $'test/testcase/($category)')
    let testcases = (testcase get $category)
    let compiler = (search executable slimec (git-ws))
    let n = ($testcases | par-each {|e| $e.id | into int | into string | str length} | sort | last)
    let max_length = ($testcases | par-each {|e| $e.name | str length} | sort | last)
    mut total_time_used = 0us
    mut failed_testcases = []
    if not $quiet {
        let p1 = ('[====]' | decorate -B)
        let p2 = ($testcases | length | decorate -B -I)
        let p3 = ('testcases from' | decorate -B)
        let p4 = ($category | decorate -B -I)
        print $"($p1) ($p2) ($p3) ($p4)"
    }
    for c in $testcases {
        let should_print = ((not $quiet) and (not $print_failed_only))
        let no = ($c.id | into int | fill -a r -w $n | decorate -w -B)
        if $should_print {
            let status = $"[TEST] ($no): ($c.name | decorate -U)"
            print -n $"($status)\r"
        }
        let result = if true {
            let source = $'($dir)/($c.id)_($c.name).sy'
            timing invoke { nu -c $"^($compiler) ($source) ($arguments)" }
        }
        let pass = ($result.exit_code == 0)
        let should_print = ((not $quiet) and ((not $print_failed_only) or (not $pass)))
        let suffix = if $should_print {
            let blank = ('' | fill -w ($max_length + 4 - ($c.name | str length)))
            let prefix = $"((if $pass { '#' } else { '$' }) | decorate -y -B) ('=' | decorate -B) "
            let value = if $pass {
                $result.time_used | decorate -y
            } else if $result.exit_code >= 0 {
                $result.exit_code | decorate -g -B
            } else {
                $result.exit_code | decorate -r -B
            }
            $blank + $prefix + $value
        }
        let status = if $should_print {
            let test_status = if $pass {
                'PASS' | decorate -g -B
            } else {
                'FAIL' | decorate -r -B
            }
            let title = if $pass {
                $"($no): ($c.name)"
            } else {
                $"($no): ($c.name | decorate -U)"
            }
            $"[($test_status)] ($title)($suffix)"
        }
        if $should_print {
            print $status
            if (not $pass) and (not ($result.stderr | is-empty)) {
                print ($result.stderr | str trim)
            }
        }
        $total_time_used = $total_time_used + ($result.time_used | into duration)
        if not $pass {
            $failed_testcases = ($failed_testcases
                | append ($c
                    | select id name
                    | insert exit_code $result.exit_code
                    | insert time_used ($result.time_used | into duration)
                    )
                )
        }
    }
    let n_fail = ($failed_testcases | length)
    let n_pass = (($testcases | length) - $n_fail)
    if not $quiet {
        let p1 = ('[----]' | decorate -B)
        let p2 = if $n_fail == 0 {
            let p1 = ('all testcases from' | decorate -B)
            let p2 = ($category | decorate -B -I)
            let p3 = ('passed' | decorate -B)
            $"($p1) ($p2) ($p3)"
        } else {
            let p1 = ($n_pass | decorate -g -B)
            let p2 = ('PASSED' | decorate -B)
            let p3 = ($n_fail | decorate -r -B)
            let p4 = ('FAILED' | decorate -B)
            $"($p1) ($p2) ($p3) ($p4)"
        }
        print $"($p1) ($p2)"
        let p3 = ('[^^^^] total time used' | decorate -B)
        let p4 = ($total_time_used | into duration -c ms | decorate -y -B -I)
        let p5 = ('average' | decorate -B)
        let p6 = ($total_time_used / ($testcases | length) | into duration -c ms | decorate -y -B -I)
        print $"($p3): ($p4) ($p5): ($p6)"
    } else {
        {
            category: $category,
            total_testcases: ($testcases | length),
            total_passed: $n_pass,
            total_failed: $n_fail,
            total_time_used: $total_time_used,
            failed_testcases: $failed_testcases,
        }
    }
}

# Compile SysY sources to llvm-ir.
def "compile ir" [
    target: string@"testcase list", # Testcase category
    builddir: string = "build",     # Path to build objects
    force: bool = false,            # Force re-compile
    silent: bool = false,           # Not print messages
] {
    if not ($builddir | path exists) {
        mkdir $builddir
    }

    let testcases = (testcase get $target)
    let total = ($testcases | length)

    def "get hint" [e] {
        $'[(($e.id | into int) + 1)/($total)] compile ($e.id)_($e.name).sy'
    }

    let maxlen = ($testcases
        | each {|e| get hint $e | str length}
        | math max
        )

    $testcases
        | each {|e|
            let name = $"($e.id)_($e.name)"
            let src = $"(git-ws)/test/testcase/($target)/($name).sy"
            let out = $"($env.PWD)/($builddir)/($name).ll"
            if not $silent {
                print -n ("\r" + (get hint $e | fill -w ($maxlen)) | decorate -B)
            }
            mut ok = true
            if (not ($out | path exists)) or $force {
                let result = (do -i { ^(search executable slimec) --emit-ir $src } | complete)
                if $result.exit_code == 0 {
                    $result.stdout | save -f $out
                } else  {
                    $ok = false
                    if not $silent {
                        let msg = ($result.stderr | str trim)
                        let msg = $msg + (char nl) + ($"program exit with ($result.exit_code)" | decorate -r -B)
                        print $"(char nl)($msg | str trim)"
                    }
                }
            }
            $e | insert compiled $ok | insert ir (if $ok { $out })
        }
        | flatten
}

# Build executables from generated IR sources.
def "build executable" [
    builddir: string = "build", # Path to build objects
    force: bool = false,        # Force re-compile
    silent: bool = false,       # Not print messages
] {
    let testcases = (each {|e| $e})
    let total = ($testcases | length)

    if not ($builddir | path exists) {
        mkdir $builddir
    }

    def "get hint" [index, e] {
        $'[(($index | into int) + 1)/($total)] build ($e.id)_($e.name).exe'
    }

    let maxlen = ($testcases
        | enumerate
        | each {|e| get hint $e.index $e.item | str length}
        | math max
        )

    $testcases
        | enumerate
        | each {|e|
            let index = $e.index
            let e = $e.item
            let bin = $"($env.PWD)/($builddir)/($e.id)_($e.name).exe"
            mut ok = $e.compiled
            if not $silent {
                print -n ("\r" + (get hint $index $e | fill -w ($maxlen)) | decorate -B)
            }
            if $ok and (((not ($bin | path exists)) or $force)) {
                let result = (do -i {
                    ^gcc -m32 $e.ir $"(git-ws)/test/lib/sylib.c" -o $bin
                } | complete)
                if $result.exit_code != 0 {
                    let msg = (($result.stderr + (char nl)) | str trim)
                    if not $silent {
                        print $"(char nl)($msg)"
                    }
                    $ok = false
                }
            }
            $e | insert executable (if $ok { $bin })
        }
}

# Run test on given testcase category.
def "run test" [
    target: string@"testcase list", # Testcase category
    silent: bool = false,           # Not print messages
] {
    let testcases = (each {|e| $e})

    def "fmt output" [e] {
        $e  | split row (char cr)
            | split row (char nl)
            | filter {|e| not ($e | is-empty)}
            | str join (char nl)
            | str trim
    }

    $testcases
        | par-each {|e|
            let compiled = $e.compiled
            let bin = $e.executable
            let expected = if $e.sample_out {
                fmt output (open $"(git-ws)/test/testcase/($target)/($e.id)_($e.name).out")
            }
            mut output = ""
            mut message = ""
            mut pass = true
            if not $compiled {
                $message = "failed to compile IR code from SysY source code"
                $pass = false
            } else if ($bin | is-empty) {
                $message = "failed to build the executable from IR"
                $pass = false
            } else if $e.sample_in {
                let sample = $"(git-ws)/test/testcase/($target)/($e.id)_($e.name).in"
                let result = (do -i { open $sample | ^$bin } | complete)
                $output = (fmt output $"($result.stdout)(char nl)($result.exit_code)")
                $pass = ($output == $expected)
            } else {
                let result = (do -i { ^$bin } | complete)
                $output = (fmt output $"($result.stdout)(char nl)($result.exit_code)")
                $pass = ($output == $expected)
            }
            if not $silent {
                print (($"complete testcase [($e.id)] ($e.name)" | decorate -B) + (if $pass {
                    " PASS " | decorate -g -B
                } else {
                    " FAIL " | decorate -r -B
                }))
            }
            $e | insert message $message | insert pass $pass | insert info {
                expected: $expected,
                output: $output,
            }
        }
        | sort-by id
}

# Build testcases.
export def "testcase build" [
    category: string@"testcase list",   # Testcase category
    builddir: string = "build",         # Path to build objects
    --force (-f),                       # Force re-compile
    --silent (-q),                      # Not print messages
    --run (-r),                         # Run testcase
    --dump
] {
    mut result = (compile ir $category $"($builddir).src" $force $silent)
    if not $silent {
        print ("\nir compile done" | decorate -g -B)
    }
    $result = ($result | build executable $"($builddir).bin" $force $silent)
    if not $silent {
        print ("\nexecutable build done" | decorate -g -B)
    }
    if $run {
        $result = ($result | run test $category $silent)
    }
    if $dump {
        $result | to json | save -f $"testcase_($category).json"
    }
    $result
}

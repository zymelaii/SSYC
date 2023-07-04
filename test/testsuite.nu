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
            timing invoke { ^$compiler $source }
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
                print $result.stderr
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

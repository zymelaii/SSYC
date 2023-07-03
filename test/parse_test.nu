#!/usr/bin/env nu

def main [] {
    (ls ./test/testcase/*.sy | get name) | each {|e|
        let result = (do -i { slimec $e } | complete)
        return { testcase: $e,  status: $result.exit_code, output: $result.stderr }
    } | filter {|e| $e.status != 0}
    #echo $'PASS ($test | filter {|e| ($e.output | str length) == 0} | length)/($test | length)'
}

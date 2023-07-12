use lib.nu *
use testsuite.nu *

def main [
    --total-runs (-n): int = 4,
] {
    print (' ID  TESTCASE    MY PERFORMANCE             O2 PERFORMANCE     SPEED RATIO' | decorate -B)
    print ('' | fill -w 74 -c '=' | decorate -B)
    let testcases = (testcase get performance)
    0..4 | each {|e|
            $testcases | par-each {|e|
                let sampleIn = $'test\testcase\performance\($e.id)_($e.name).in'
                let binO2 = $'build.gcc\($e.id)_($e.name).exe'
                let binMe = $'build.bin\($e.id)_($e.name).exe'
                let timeO2 = (open $sampleIn | timeit { do -i { ^$binO2 } | complete })
                let timeMe = (open $sampleIn | timeit { do -i { ^$binMe } | complete })
                let p1 = $"($'[($e.id)]' | decorate -B) ($e.name | fill -w 8 | decorate -y -B)"
                let p2 = ($"($timeMe)" | fill -w 24 | decorate -B)
                let p3 = "Vs."
                let p4 = ($"($timeO2)" | fill -w 24 | decorate -B)
                let rate = ((($timeO2 | into int) * 10000.0 / ($timeMe | into int) | math round) / 100.0)
                let p5 = if $rate >= 100.0 {
                    $"($rate)%" | decorate -g -B
                } else {
                    $"($rate)%" | decorate -r -B
                }
                print $"($p1) ($p2) ($p3) ($p4) ($p5)"
                {
                    id: $e.id,
                    name: $e.name,
                    time_me: $timeMe,
                    time_o2: $timeO2,
                    rate: $rate,
                }
            }
        }
        | flatten
        | group-by name

    print ('' | fill -w 74 -c '-' | decorate -B)
}

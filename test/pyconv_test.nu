#!/usr/bin/env nu

# SysY-Python 编译测试

def main [SSYC_EXECUTABLE: string] {
	let TEST_DIR = ($env.FILE_PWD | path join)
	let SAMPLES = (ls $'($TEST_DIR)/functional/*.sy' | get name)

	let total = ($SAMPLES | length)
	mut failed = 0

	echo $'[============] Running ($total) test from 1 test case'

	for test_in in $SAMPLES {
		echo $'[ RUN        ] ($test_in)'

		let resp = (do { ^$SSYC_EXECUTABLE --conv2py --input $test_in } | complete)

		let this_case = ($test_in | rg -oP -e '([0-9a-zA-Z_]*)(?=\.sy$)')
		let fin = $'($TEST_DIR)/functional/($this_case).in'
		let fout = $'($TEST_DIR)/functional/($this_case).out'

		let expected_output = (open $fout | into string | str trim)

		mut passed = ($resp.exit_code == 0)
		mut ans = ''

		if ($fin | path exists) {
			let func_resp = (do { open $fin | python -c $resp.stdout } | complete)
			$passed = ($passed and $func_resp.exit_code == 0)
			$ans = ($func_resp.stdout | into string | str trim)
		} else {
			let func_resp = (do { $resp.stdout | python } | complete)
			$passed = ($passed and $func_resp.exit_code == 0)
			$ans = ($func_resp.stdout | into string | str trim)
		}

		$passed = ($passed and $ans == $expected_output)

		if not $passed {
			$failed = $failed + 1
			echo '[         OK ] failed'
		} else {
			echo '[         OK ] passed'
		}
	}

	echo $'[------------] ($total) test from 1 test case ran'
	if $failed == 0 {
		echo $'[   PASSED   ] ($total) test'
	} else {
		echo $'[   FAILED   ] ($failed) test'
	}
	echo '[============]'
}

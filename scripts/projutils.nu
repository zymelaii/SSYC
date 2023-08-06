use lib.nu git-ws

export def FlattenProject [
    target: string = "srcs.flatten"
] {
    let target = $'($env.PWD)(char psep)($target)'
    mkdir $target

    let root = (git-ws)
    let src_dir = $'($root)(char psep)src'

    let sources = (fd --search-path $'($src_dir)(char psep)slime' -e c -e cpp -e h -e def
        | lines
        | each {|e|
            $e  | path relative-to $src_dir
                | path split
                | str join '/'
        }
        | append main.cpp
        )

    let rename_table = ($sources
        | enumerate
        | each {|e|
            let id = $e.index
            let source = $e.item
            let target = $'($id).($source | path parse | get extension)'
            { source: $source, target: $target }
        }
        )

    $rename_table | each {|item|
        let source = (open $'($src_dir)(char psep)($item.source)')
        let cur_dir = ($item.source | path dirname)
        let abs_include = ($source
            | rg -n '#include <'
            | lines
            | each {|e|
                $e | parse -r '^(?<line>\d+):.*?<(?<abs_header>.*)>$'
            }
            | flatten
            | filter {|e| $e.abs_header in $sources}
            | each {|e|
                let target = ($rename_table
                    | where source == $e.abs_header
                    | first
                    | get target
                    )
                { line: $e.line, target: $target }
            }
            | flatten
            )
        let rel_include = ($source
            | rg -n '#include "'
            | lines
            | each {|e|
                $e | parse -r '^(?<line>\d+):.*?"(?<rel_header>.*)"$'
            }
            | flatten
            | each {|e|
                let target = ($rename_table
                    | where source == $'($cur_dir)/($e.rel_header)'
                    | first
                    | get target
                    )
                { line: $e.line, target: $target }
            }
            | flatten
            )
        let replace_list = ($abs_include | append $rel_include | sort-by -n line)
        let sloc = ($source
            | lines
            | enumerate
            | each {|e|
                let line = $e.index
                let result = ($replace_list | where line == ($line + 1 | into string))
                if ($result | is-empty) {
                    $e.item
                } else {
                    $'#include "($result | first | get target)"'
                }
            }
            | flatten
            )
        $"($sloc | str join (char nl))(char nl)" | save -f $"($target)(char psep)($item.target)"
        print $'complete ($item.source) -> ($item.target)'
    } | ignore
}

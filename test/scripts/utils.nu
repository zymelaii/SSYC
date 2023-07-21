# Throw an error with the given information.
export def throw [
    error: string,      # Error type
    message: string,    # Extra message
    meta: any,          # Metadata with span for the error hint
] {
    let span = $meta.span
    error make {
        msg: $error,
        label: {
            text: $message,
            start: $span.start,
            end: $span.end,
        },
    }
}

# Decorate text with style.
export def decorate [
    --black (-0),       # Set color black
    --white (-w),       # Set color white
    --red (-r),         # Set color red
    --green (-g),       # Set color green
    --blue (-b),        # Set color blue
    --yellow (-y),      # Set color yellow
    --mag (-m),         # Set color mag
    --cyan (-c),        # Set color cyan
    --bold (-B),        # Set bold
    --underline (-U),   # Set underline
    --italic (-I)       # Set italic
    --inverse (-R),     # Set inverse
    --no-reset (-1),    # No reset after decoration
] {
    let text = ($in | into string)
    if ($text | describe) != 'string' {
        throw ArgumentError "decorate only accept string as input" (metadata $text)
    }
    let style_code = ['30' '37' '31' '32' '34' '33' '35' '36' '1' '4' '3' '7']
    let switchs = [$black $white $red $green $blue $yellow $mag $cyan $bold $underline $italic $inverse]
    let style = (0..(($style_code | length) - 1)
        | filter {|i| $switchs | get $i}
        | each {|i| $style_code | get $i}
        | str join ';'
        )
    if $no_reset {
        $"\e[($style)m($text)"
    } else {
        $"\e[($style)m($text)\e[0m"
    }
}

import os

project = "HorizonOS"
author = "David Santamaria"
copyright = "2026, David Santamaria"

extensions = [
    "breathe",
    "exhale", "myst_parser",
]

source_suffix = {
    ".rst": "restructuredtext",
    ".md": "markdown",
}

exclude_patterns = ["_build", "doxygen", "book", "src", "themes"]

html_theme = "furo"
html_title = "Horizon"
html_static_path = []

# point breathe at doxygen xml output.
breathe_projects = {"Horizon": "./doxygen/xml"}
breathe_default_project = "Horizon"
breathe_default_members = ("members",)

# have exhale auto-generate an API tree from the doxygen xml
exhale_args = {
    "containmentFolder":     "./api",
    "rootFileName":          "library_root.rst",
    "doxygenStripFromPath":  "..",
    "rootFileTitle":         "API Reference",
    "createTreeView":        True,
    "contentsDirectives":    False,
    "exhaleExecutesDoxygen": True,
    "exhaleDoxygenStdin": """
        PROJECT_NAME      = Horizon
        INPUT             = ../kernel ../libc
        FILE_PATTERNS     = *.c *.h
        RECURSIVE         = YES
        EXTRACT_ALL       = YES
        EXTRACT_STATIC    = NO
        GENERATE_HTML     = YES
        GENERATE_LATEX    = NO
        GENERATE_XML      = YES
        XML_OUTPUT        = xml
        XML_PROGRAMLISTING = YES
        EXCLUDE_SYMBOLS   = __* _*
        EXCLUDE_PATTERNS  = */limine.h */uacpi/* */uacpi*
        EXCLUDE           = ../uacpi
    """,
}

primary_domain = "c"
highlight_language = "c"

myst_enable_extensions = ["colon_fence", "deflist"]

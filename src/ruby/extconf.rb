# Loads mkmf which is used to make makefiles for Ruby extensions
require 'mkmf'

# Give it a name
extension_name = 'Pipes'

# The destination
dir_config(extension_name)

$srcs = ['../src/scheme.c', 'Pipes.rb.c']
$objs = ['../src/scheme.o', 'Pipes.rb.o']
# Do the work
create_makefile(extension_name)

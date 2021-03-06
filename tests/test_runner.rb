require 'test/unit'

def run_script(binary)
  cmd = "#{binary} > /dev/null 2>&1"
  system cmd
  $?.success?
end

class NoomrTests < Test::Unit::TestCase
  ARGV.each{ |binary|
    define_method("#{binary}") {

      assert_equal true, run_script(binary)
    }
  }
end

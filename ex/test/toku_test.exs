defmodule TokuTest do
  use ExUnit.Case
  doctest Toku

  test "greets the world" do
    assert Toku.hello() == :world
  end
end

defmodule Toku.TestServer do
  @behaviour Toku.Server
end

defmodule TokuTest do
  use ExUnit.Case
  doctest Toku

  test "server init" do
    {:ok, pid} = Toku.TestServer.init()
  end
end

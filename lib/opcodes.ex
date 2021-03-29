defmodule Toku.Opcodes do

  defmacro __using__(_) do
    quote do
      @op_hello 1
      @op_hello_ack 2
      @op_ping 3
      @op_pong 4
      @op_request 5
      @op_response 6
      @op_push 7
      @op_goaway 8
      @op_error 9
    end
  end

end

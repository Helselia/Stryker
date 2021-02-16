# Field     Type      Description
# -------------------------------
# op        integer   opcode (9 for SocketError)
# f         integer   flags (defined in `lib/toku/sockets/flags.ex`)
# seq       integer   sequence num
# code      intege    socket close code

defmodule Toku.Sockets.SocketError do
  @derive [Poison.Encoder]

  @type t :: %{
    op: Integer,
    f: Integer,
    seq: Integer,
    code: Integer
  }

  defstruct op: 9,
            f: 0,
            seq: 0,
            code: 0

  def new(code) do
    %Toku.Sockets.SocketError{
      code: code
    }
  end
end

defmodule Toku.Sockets.SocketErrorCode do
  # Unknown
  def unknown, do: 0
  def unknown_op, do: 1000

  # Missing
  def missing_op, do: 2000
  def missing_d, do: 2001
  def missing_f, do: 2002
  def missing_seq, do: 2003
  def missing_v, do: 2004

  # Common
  def repeated_hello, do: 3000
end

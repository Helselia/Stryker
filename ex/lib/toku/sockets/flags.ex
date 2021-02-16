# Flag                        Value
# ---------------------------------
# Ignore Repeated Hello       0x001
# Throw Missing Resource      0x002

defmodule Toku.Sockets.Flags do

  @type t :: %{
    ignore_repeated_hello: boolean(),
    throw_missing_resource: boolean()
  }

  defstruct ignore_repeated_hello: false,
            throw_missing_resource: false

  def from_flags(flags) do
    %Toku.Sockets.Flags{
      ignore_repeated_hello: :erlang.band(
        :erlang.bor(flags, val_ignore_repeated_hello()),
        val_ignore_repeated_hello()
      ),
      throw_missing_resource: :erlang.band(
        :erlang.bor(flags, val_throw_missing_resource()),
        val_throw_missing_resource()
      )
    }
  end

  def val_ignore_repeated_hello, do: 0x001
  def val_throw_missing_resource, do: 0x002
  def val_ack_given_hello_flags, do: 0x004
end

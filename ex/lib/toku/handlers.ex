defmodule Toku.Sockets.Handlers do
  def handle_1(flags, _data, state) do
    if Map.has_key?(state, :seq) do
      if not state[:ignore_repeated_hello] do
        throw(%{code: Toku.Sockets.SocketErrorCode.repeated_hello})
      end
    end

    f = Map.from_struct(Toku.Sockets.Flags.from_flags(flags))

    {%{
      op: 2,
      flags: Toku.Sockets.Flags.val_ack_given_hello_flags(),
      p: 2500
    }, Map.merge(Map.merge(state, f), %{ seq: 1 })}
  end
end

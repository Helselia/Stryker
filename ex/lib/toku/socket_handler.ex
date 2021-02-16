# Registry.Toku
# |> Registry.dispatch(state[:registry_key], fn(entries) ->
#     for {pid, _} <- entries do
#       if pid != self() do
#         Process.send(pid, "message", [])
#       end
#     end
#   end)

defmodule Toku.SocketHandler do
  @behaviour :cowboy_websocket

  def init(request, _state) do
    state = %{registry_key: request[:headers]["sec-websocket-key"]}

    {:cowboy_websocket, request, state}
  end

  def websocket_init(state) do
    Registry.Toku
    |> Registry.register(state[:registry_key], {})

    {:ok, state}
  end

  def websocket_handle({:text, json}, state) do
    payload = Poison.decode!(json)

    if not payload["op"] do
      {:reply, {:text, Poison.encode!(Toku.Sockets.SocketError.new(Toku.Sockets.SocketErrorCode.missing_op))}, state}
    else
      try do
        if payload["op"] in opcode_list() do
          handle_op(payload["op"], payload["f"], payload)
        else
          throw(%{code: Toku.Sockets.SocketErrorCode.unknown_op})
        end
      catch
        %{code: code} ->
          {:reply, {:text, Poison.encode!(Toku.Sockets.SocketError.new(code))}, state}
        _ ->
          {:reply, {:text, Poison.encode!(Toku.Sockets.SocketError.new(Toku.Sockets.SocketErrorCode.unknown))}, state}
      end
    end
  end

  def websocket_info(info, state) do
    {:reply, {:text, info}, state}
  end

  def terminate(_reason, _req, _state) do
    :ok
  end

  defp opcode_list() do
    [
      1, # HELLO
      3, # PING
      4, # PONG
      5, # REQUEST
      7, # PUSH
    ]
  end

  defp handle_op(opcode, flags, opts) do

  end
end

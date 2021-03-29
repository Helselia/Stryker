defmodule Toku.Server do
  defmodule Http do
    defmodule State do
      defstruct [:handler, :path, :transport, :socket, :buffer, :handler_opts]

      def new(socket, transport, opts) do
        %__MODULE__{
          path: Keyword.fetch!(opts, :toku_path),
          handler: Keyword.fetch!(opts, :handler),
          socket: socket,
          transport: transport,
          buffer: "",
          handler_opts: Keyword.get(opts, :handler_opts, %{})
        }
      end
    end

    require Logger
    alias Toku.RanchProtocol

    @behaviour :ranch_protocol
    @max_payload 1024 * 100

    def start_link(ref, socket, transport, opts) do
      pid = spawn_link(__MODULE__, :init, [{ref, socket, transport, opts}])
      {:ok, pid}
    end

    def init({ref, socket, transport, opts}) do
      state = State.new(socket, transport, opts)

      transport_opts =
        opts
        |> Keyword.get(:transport_opts, [])
        |> Keyword.get(:active, true)

      # This will fail if the path hasn't been set.
      _toku_path = Keyword.fetch!(opts, :toku_path)
      :ok = :ranch.accept_ack(ref)
      transport.setopts(socket, transport_opts)

      loop(state)
    end

    def loop(%{socket: sock} = state) do
      receive do
        {:tcp, ^sock, data} ->
          case handle_tcp_data(data, state) do
            :ok ->
              exit(:normal)

            {:error, {:not_complete, request}} ->
              loop(%State{state | buffer: request})

            {:error, reason} ->
              Logger.info(
                "TCP error #{inspect(reason)} from client #{inspect(ip_address(sock))}. Closing"
              )

              exit(reason)
          end

        {:tcp_error, ^sock, reason} ->
          Logger.warn(
            "TCP error #{inspect(reason)} from client #{inspect(ip_address(sock))}. Closing"
          )

          exit(reason)

        {:tcp_closed, ^sock} ->
          Logger.info("Client #{inspect(ip_address(sock))} closed.")
          exit(:normal)
      end
    end

    defp handle_tcp_data(
      extra_data,
      %{socket: sock, path: toku_path, transport: transport, buffer: buffer} = state
    ) do
      with {:ok, ^toku_path, {headers, _}} <- try_parse_request(buffer <> extra_data),
          header_val when is_bitstring(header_val) <- :proplists.get_value("upgrade", headers),
          "toku" <- String.downcase(header_val) do
        upgrade_headers = [{"connection", "Upgrade"}, {"upgrade", "toku"}]
        response = :cow_http.response(101, :"HTTP/1.1", upgrade_headers)
        transport.send(sock, response)
        RanchProtocol.upgrade(sock, transport, state.handler, state.handler_opts)
      else
        {:error, :payload_too_large} ->
          response = :cow_http.response(413, :"HTTP/1.1", [{"connection", "close"}])
          transport.send(sock, response)
          {:error, :payload_too_large}

        {:error, _} = err ->
          err

        _ ->
          response = :cow_http.response(404, :"HTTP/1.1", [{"connection", "close"}])
          transport.send(sock, response)
          :ok
      end
    end

    def try_parse_request(data) when byte_size(data) >= @max_payload, do: {:error, :payload_too_large}

    def try_parse_request(data) do
      with true <- String.ends_with?(data, "\r\n\r\n"),
          ["GET" <> rest_of_line, rest] <- String.split(data, "\r\n", parts: 2),
          [path, _version] <- String.split(rest_of_line, " ", parts: 2) do
        {:ok, path, :cow_http.parse_headers(rest)}
      else
        _ ->
          {:error, {:not_complete, data}}
      end
    end

    defp ip_address(sock) do
      with {:ok, {ip, _port}} <- :inet.peername(sock) do
        :inet_parse.ntoa(ip)
      end
    end
  end

  @type transport :: :ranch_tcp | :ranch_ssl
  @type transport_option :: :gen_tcp.option() | :ranch.opts()
  @type option ::
          {:toku_path, String.t()}
          | {:transport_opts, [transport_option]}
          | {:handler_opts, Keyword.t()}
          | {:transport, transport}
  @type options :: [{:handler, module} | [option]]

  @type tcp_port :: 0..65535
  @type path :: String.t()

  @spec start_link(tcp_port, path, module, options) :: {:ok, pid} | {:error, any}
  def start_link(port, path, handler, opts \\ []) do
    server_name = Keyword.get(opts, :server_name, :toku)
    transport = Keyword.get(opts, :transport, :ranch_tcp)

    opts =
      opts
      |> Keyword.put(:handler, handler)
      |> Keyword.put(:toku_path, path)

    {transport_opts, opts} = Keyword.pop(opts, :transport_opts, [])
    transport_opts = Keyword.put(transport_opts, :port, port)

    :ranch.start_listener(server_name, transport, transport_opts, Http, opts)
  end

  defdelegate stop(listener), to: :ranch, as: :stop_listener
end

defmodule Toku do
  use Application

  def start(_type, _args) do
    children = [
      {Plug.Cowboy, [scheme: :http, plug: Toku.Router, options: [dispatch: dispatch(), port: 4999]]},
      {Registry, [keys: :duplicate, name: Registry.Toku]}
    ]

    opts = [strategy: :one_for_one, name: Toku.Application]

    IO.puts "Starting Toku Link"

    Supervisor.start_link(children, opts)
  end

  defp dispatch do
    [
      {:_,
        [
          {:_, Toku.SocketHandler, []}
        ]
      }
    ]
  end
end

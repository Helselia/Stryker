defmodule Toku.Router do
  use Plug.Router

  plug :match
  plug Plug.Parsers,
    parsers: [:json],\
    pass: ["application/json"],
    json_decoder: Poison
  plug :dispatch

  get "/" do
    conn
    |> send_resp(200, Poison.encode!(%{
      code: 0,
      message: "200: OK"
    }))
  end

  match _ do
    conn
    |> send_resp(404, Poison.encode!(%{
      code: 0,
      message: "404: Not Found"
    }))
  end

end

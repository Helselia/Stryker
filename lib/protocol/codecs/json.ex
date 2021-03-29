if elem(Code.ensure_compiled(:jiffy), 0) != :error do
  defmodule Toku.Protocol.Codecs.Json do
    @behaviour Toku.Protocol.Codec

    @encode [:use_nil, bytes_per_iter: 4096]
    @decode [:use_nil, :return_maps, bytes_per_iter: 4096]

    def name, do: "json"

    def encode(term), do: :jiffy.encode(term, @encode)

    def decode(term), do: :jiffy.decode(term, @decode)

  end
end

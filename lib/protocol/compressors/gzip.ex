defmodule Toku.Protocol.Compressors.Gzip do

  @behaviour Toku.Protocol.Compressor

  def name, do: "gzip"

  def compress(iodata), do: :zlib.gzip(iodata)

  def decompress(iodata), do: :zlib.gunzip(iodata)

end

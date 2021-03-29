defmodule Toku.Protocol.Compressors.NoOp do

  @behaviour Toku.Protocol.Compressor

  def name, do: ""

  def compress(binary), do: binary

  def decompress(binary), do: binary

end

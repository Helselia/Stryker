defmodule Toku.Protocol.Compressors do

  @enabled_compressors [NoOp, Gzip]
                       |> Enum.map(&Module.concat(__MODULE__, &1))
                       |> Enum.filter(fn mod -> elem(Code.ensure_compiled(mod), 0) != :error end)

  def all do
    @enabled_compressors
  end

end

defmodule Toku.Protocol.Codecs do

  @enabled_codecs [Erlpack, Json]
                  |> Enum.map(&Module.concat(__MODULE__, &1))
                  |> Enum.filter(fn mod -> elem(Code.ensure_compiled(mod), 0) != :error end)

  def all do
    @enabled_codecs
  end

end

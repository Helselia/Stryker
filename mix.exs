defmodule Toku.MixProject do
  use Mix.Project

  @project_url "https://github.com/Helselia/Toku.git"

  @version "0.0.1"

  def project do
    [
      app: :toku,
      version: @version,
      elixir: "~> 1.11",
      elixirrc_paths: ["ex/lib/"],
      test_paths: ["ex/test/"],
      start_permanent: Mix.env() == :prod,
      deps: deps(),
      description: "An RPC Websocket Server/Client based off of Loqui",
      package: package(),
      source_url: @project_url,
      homepage_url: @project_url
    ]
  end

  # Run "mix help compile.app" to learn about applications.
  def application do
    [
      extra_applications: [:logger]
    ]
  end

  # Run "mix help deps" to learn about dependencies.
  defp deps do
    [
      {:ranch, "~> 1.4.0"},
      {:jiffy, "~> 1.0.8", optional: true},
      {:cowlib, "~> 2.10.1"},
      {:connection, "~> 1.1"}
    ]
  end

  defp package do
    [
      name: :toku,
      files: ~w(README.md mix.exs ex/lib/*),
      maintainers: ["Constanze Amalie"],
      licenses: ["MIT"],
      links: %{
        "GitHub" => @project_url
      }
    ]
  end
end

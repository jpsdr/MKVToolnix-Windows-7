#!/usr/bin/ruby -w

# T_520truehd_mlp_atmos_detection
describe "mkvmerge / identification of TrueHD Atmos"

def verify520 file, track, codec
  file = "data/truehd/#{file}"

  test "testing #{file}" do
    output, _     = identify file, :format => :json
    output        = output.join ''
    json          = JSON.parse(output)
    valid, errors = json_schema_validate(JSON.load(output))
    ok            = false

    if !valid
      puts " JSON validation errors in #{file}:"
      puts errors.map { |err| "  #{err}" }.join("\n")

    elsif json["tracks"][track]["codec"] == codec
      ok = true

    end

    json.delete("identification_format_version")
    JSON.dump(json).md5 + "+#{ok}"
  end
end

verify520 "god-save-the-queen.mlp",                           0, "MLP"

verify520 "blueplanet.thd",                                   0, "TrueHD"
verify520 "blueplanet.thd+ac3",                               0, "TrueHD"

verify520 "Dolby_Amaze_Lossless-ATMOS-thedigitaltheater.thd", 0, "TrueHD Atmos"
verify520 "dolby_atmos_truehd_sample.m2ts",                   1, "TrueHD Atmos"
verify520 "truehd-atmos+ac3.m2ts",                            0, "TrueHD Atmos"

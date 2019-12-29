#!/usr/bin/ruby -w

# T_685propedit_track_uid_changes

describe "mkvpropedit / changing track UIDs and referring UIDs in chapters & tags"

test "change track UIDs" do
  sys "cp data/mkv/propedit-track-uid-changes.mkv #{tmp}"

  propedit tmp, "--edit track:1 --set track-uid=4711 --edit track:2 --set track-uid=815"

  chapters, _ = sys("../src/mkvextract #{tmp} chapters -", :no_result => true)
  tags,     _ = sys("../src/mkvextract #{tmp} tags -",     :no_result => true)

  [
    chapters.join("").scan(%r{<ChapterTrackNumber>(\d+)}).flatten.join('+'),
    tags.    join("").scan(%r{<TrackUID>(\d+)}).          flatten.join('+')
  ].join('+')
end

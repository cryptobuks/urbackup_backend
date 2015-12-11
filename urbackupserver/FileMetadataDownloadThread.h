#pragma once
#include "../urbackupcommon/fileclient/FileClient.h"
#include "../Interface/Thread.h"
#include "server_prepare_hash.h"
#include "../urbackupcommon/chunk_hasher.h"
#include "server_log.h"
#include <memory>

namespace server
{

namespace
{
	struct SFolderItem
	{
		SFolderItem()
			: created(0), modified(0),
			  accessed(0), folder_items(-1),
			  counted_items(0)
		{

		}

		std::string path;
		std::wstring os_path;
		int64 created;
		int64 modified;
		int64 accessed;
		int64 folder_items;
		int64 counted_items;
	};
}

class FileMetadataDownloadThread : public IThread
{
public:
	FileMetadataDownloadThread(FileClient* fc, const std::string& server_token, logid_t logid);
	FileMetadataDownloadThread(const std::string& server_token, std::wstring metadata_tmp_fn);
	~FileMetadataDownloadThread();

	virtual void operator()();

	bool applyMetadata(const std::wstring& backup_metadata_dir, const std::wstring& backup_dir,
		INotEnoughSpaceCallback *cb, const std::map<std::wstring, std::wstring>& filepath_corrections);
	bool applyWindowsMetadata(IFile* metadata_f, IFile* output_f, int64& metadata_size, INotEnoughSpaceCallback *cb, int64 output_offset);
    bool applyUnixMetadata(IFile* metadata_f, IFile* output_f, int64& metadata_size, INotEnoughSpaceCallback *cb, int64 output_offset);

	bool getHasError();
	
	void shutdown();

private:

	void addSingleFileItem(std::string dir_path);
	void addFolderItem(std::string path, const std::wstring& os_path, bool is_dir, int64 created, int64 modified, int64 accessed, int64 folder_items);

	std::vector<SFolderItem> saved_folder_items;

	std::auto_ptr<FileClient> fc;
	const std::string& server_token;

	std::vector<char> buffer;

	bool has_error;
	std::wstring metadata_tmp_fn;
	logid_t logid;

	bool dry_run;
};

int check_metadata();

} //namespace server

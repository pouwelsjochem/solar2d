//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Solar2D game engine.
// With contributions from Dianchu Technology
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

package com.ansca.corona.storage;


/**
 * Provides easy seamless access to files that are external to the application and files within the APK's
 * "assets" directory. Also provides simple methods
 * for copying, moving, and opening these files as well as fetching their extensions and mime types.
 * <p>
 * All methods in this class are thread safe and can be called from any thread.
 */
public class FileServices extends com.ansca.corona.ApplicationContextProvider {
	/** Provides fast access to file entries within an APK file, which is really a zip file. */
	private static ZipResourceFile sApkZipEntryReader = null;

	/**
	 * For more efficient I/O throughput, BUFFER_SIZE at least 4KB is needed.
	 * For copy size 1MB and above, 64KB is recommended.
	 */
	private final static int BUFFER_SIZE_NORMAL = 4096;			// 4KB
	private final static int BUFFER_SIZE_LARGE_IO = 65536;		// 64KB
	private final static int BUFFER_SIZE_THRESHOLD = 1048576;	// 1MB

	/**
	 * Creates an object that provides easy access to the file system and this application's
	 * internal files in the APK's asset files.
	 * @param context Reference to an Android created context used to access the system's directories.
	 *                <p>
	 *                Setting this to null will cause an exception to be thrown.
	 */
	public FileServices(android.content.Context context) {
		super(context);

		// Open the APK as a zip file for fast asset file access, if not done already.
		if (sApkZipEntryReader == null) {
			synchronized (FileServices.class) {
				if (sApkZipEntryReader == null) {
					try {
						sApkZipEntryReader = new ZipResourceFile(context.getApplicationInfo().sourceDir);
					}
					catch (Exception ex) { }
				}
			}
		}
	}

	/**
	 * Determines if the given path\file name is an asset file inside of the APK file.
	 * @param filePath The path and file name to check if it is an asset file. Cannot be null or empty.
	 * @return Returns true if the given file is inside of the APK file.
	 *         <p>
	 *         Returns false if the given file is an external file, null, or an empty string.
	 */
	public boolean isAssetFile(String filePath) {
		// Return false if given an invalid file name.
		if ((filePath == null) || (filePath.length() <= 0)) {
			return false;
		}
		
		// Assume it is an asset file if given a relative path or just a file name.
		if (filePath.startsWith(java.io.File.separator) == false) {
			return true;
		}
		
		// Assume the given file is not in the assets directory.
		return false;
	}

	/**
	 * Determines if the given file exists in the application's assets directory.
	 * @param filePath Relative path to the asset file.
	 * @return Returns true if the file exists in this application's assets directory.
	 *         <p>
	 *         Returns false if the file could not be found.
	 */
	public boolean doesAssetFileExist(String filePath) {
		if (getAssetFileLocation(filePath) != null) {
			return true;
		}
		android.content.res.AssetManager assetManager = getApplicationContext().getAssets();
		java.io.InputStream is = null;
		try {
			is = assetManager.open(filePath, android.content.res.AssetManager.ACCESS_BUFFER);
		} catch (Throwable ignore){};
		if(is != null) {
			try {
				is.close();
			} catch (Throwable ignore) {}
			return true;
		}
		return false;
	}

	/**
	 * Determines if the given file exists in the application's resource directory
	 * @param filePath Relative path to the resource file (e.g. "drawable/image.png").
	 * @return Returns true if the file exists in this application's resource directory
	 *         <p>
	 *         Returns false if the file could not be found.
	 */
	public boolean doesResourceFileExist(String filePath) {
		ZipResourceFile.ZipEntryRO zipEntry = null;
		String entryName = null;

		if (sApkZipEntryReader != null) {
			entryName = "res/" + filePath;
			zipEntry = sApkZipEntryReader.getEntry(entryName);
		}

		return (zipEntry != null);
	}

	/**
	 * Determines size in bytes of the given file in the application's resource directory
	 * @param filePath Relative path to the resource file (e.g. "drawable/image.png").
	 * @return Returns size in bytes if the file exists in this application's resource directory
	 *         <p>
	 *         Returns -1 if the file could not be found.
	 */
	public long getResourceFileSize(String filePath) {
		ZipResourceFile.ZipEntryRO zipEntry = null;
		String entryName = null;

		if (sApkZipEntryReader != null) {
			entryName = "res/" + filePath;
			zipEntry = sApkZipEntryReader.getEntry(entryName);

			if (zipEntry != null) {
				return zipEntry.mUncompressedLength;
			}
		}

		return -1;
	}

	/**
	 * Determines where the given asset file can be found.
	 * <p>
	 * Note that assets files are packaged within an APK file.
	 * This method determines which package file contains the given asset, where the asset's bytes
	 * can be found within the package file, determines the asset's zip entry name if applicable,
	 * and whether or not the asset is compressed.
	 * <p>
	 * This information can be used to directly read the asset's bytes for fast access, provided
	 * that the asset is not compressed. This is particularly usefull if the asset needs to be
	 * accessed from native C/C++ code or from a 3rd party library that does not support Android's
	 * AssetManager class.
	 * @param filePath Relative path to the asset file.
	 * @return Returns information about where the given asset is located.
	 *         <p>
	 *         Returns null if the asset could not be found or if given an invalid argument.
	 */
	public AssetFileLocationInfo getAssetFileLocation(String filePath) {
		ZipResourceFile.ZipEntryRO zipEntry = null;
		String entryName = null;

		// Do not continue if the given file is not an asset file.
		if (isAssetFile(filePath) == false) {
			return null;
		}

		// If asset not found, then attempt to fetch it from the APK's "assets" directory.
		if ((zipEntry == null) && (sApkZipEntryReader != null)) {
			entryName = "assets/" + filePath;
			zipEntry = sApkZipEntryReader.getEntry(entryName);
		}

		// Extracted asset file if exists
		if(zipEntry == null) {
			java.io.File unpackedAsset = getCoronaResourcesFile(filePath);
			if(unpackedAsset != null && unpackedAsset.exists()) {
				AssetFileLocationInfo.Settings settings = new AssetFileLocationInfo.Settings();
				settings.setPackageFile(unpackedAsset);
				settings.setByteCountInPackage(unpackedAsset.length());
				settings.setAssetFilePath(filePath);
				settings.setZipEntryName(unpackedAsset.getAbsolutePath());
				return new AssetFileLocationInfo(settings); //Early Return!
			}
		}

		// If asset not found, then attempt to fetch it from the APK's "raw" resource directory.
		if ((zipEntry == null) && (sApkZipEntryReader != null)) {
			StringBuilder builder = new StringBuilder();
			builder.append("res/raw/");
			builder.append(createRawResourceNameForAsset(filePath));
			String extensionName = getExtensionFrom(filePath);
			if ((extensionName != null) && (extensionName.length() > 0)) {
				builder.append(".");
				builder.append(extensionName.toLowerCase());
			}
			entryName = builder.toString();
			zipEntry = sApkZipEntryReader.getEntry(entryName);
		}

		// If asset not found, then attempt to fetch it from the APK's "drawable" resource directory.
		if ((zipEntry == null) && (sApkZipEntryReader != null)) {
			StringBuilder builder = new StringBuilder();
			builder.append("res/drawable/");
			builder.append(filePath);
			entryName = builder.toString();
			zipEntry = sApkZipEntryReader.getEntry(entryName);
		}

		// Do not continue if the asset was not found up above.
		if (zipEntry == null) {
			return null;
		}

		// Return information about where the asset is located.
		AssetFileLocationInfo assetLocationInfo = null;
		try {
			AssetFileLocationInfo.Settings settings = new AssetFileLocationInfo.Settings();
			settings.setPackageFile(zipEntry.mFile);
			settings.setAssetFilePath(filePath);
			settings.setZipEntryName(entryName);
			settings.setByteOffsetInPackage(zipEntry.mOffset);
			settings.setByteCountInPackage(zipEntry.mCompressedLength);
			settings.setIsCompressed(zipEntry.isUncompressed() == false);
			assetLocationInfo = new AssetFileLocationInfo(settings);
		}
		catch (Exception ex) { }
		return assetLocationInfo;
	}

	/**
	 * Safely opens an input stream to the given file without exceptions.
	 * @param filePath The path and name of the file to be opened.
	 * @return Returns an input stream to the given file.
	 *         <p>
	 *         Returns null if unable to find or access the given file.
	 */
	public java.io.InputStream openFile(String filePath) {
		// Validate arguments.
		if ((filePath == null) || (filePath.length() <= 0)) {
			return null;
		}

		// Open a stream to the given file.
		java.io.InputStream inputStream = null;
		if (isAssetFile(filePath)) {
			// Attempt to access the file in the APK's "assets" directory.
			try {
				android.content.res.AssetManager assetManager = getApplicationContext().getAssets();
				inputStream = assetManager.open(filePath, android.content.res.AssetManager.ACCESS_BUFFER);
			}
			catch (Exception ex) { }
			if (inputStream == null) {
				// Attempt to use extracted "coronaResources" file
				try
				{
					inputStream = new java.io.FileInputStream(getCoronaResourcesFile(filePath));
				} catch (Exception ex) { }
				if (inputStream == null) {
					// Attempt to access the file in the APK's "raw" resource directory.
					try {
						String resourceName = createRawResourceNameForAsset(filePath);
						ResourceServices resourceServices = new ResourceServices(getApplicationContext());
						int resourceId = resourceServices.getRawResourceId(resourceName);
						if (resourceId != ResourceServices.INVALID_RESOURCE_ID) {
							inputStream = resourceServices.getResources().openRawResource(resourceId);
						}
					} catch (Exception ex) {
					}
				}
			}
		}
		else {
			// Open up a stream to the external file.
			try {
				inputStream = new java.io.FileInputStream(filePath);
			}
			catch (Exception ex) { }
		}
		return inputStream;
	}

	/**
	 * Safely opens an input stream to the given file without exceptions.
	 * @param file The file to be opened.
	 * @return Returns an input stream to the given file.
	 *         <p>
	 *         Returns null if unable to find or access the given file.
	 */
	public java.io.InputStream openFile(java.io.File file) {
		// Validate argument.
		if (file == null) {
			return null;
		}

		// Open the file.
		return openFile(file.getPath());
	}
	
	/**
	 * Safely opens an input stream to an entry within a zip file.
	 * @param filePath The path and name of the zip file to be opened.
	 * @param entryName The unique name of the entry within the zip file.
	 * @return Returns an input stream to the zip file's entry.
	 *         <p>
	 *         Returns null if unable to fine the given zip file or its entry.
	 */
	private java.io.InputStream openZipFileEntry(String filePath, String entryName) {
		// Validate arguments.
		if ((filePath == null) || (filePath.length() <= 0)) {
			return null;
		}
		
		// Open a stream to the zip file entry.
		java.io.InputStream inputStream = null;
		try {
			inputStream = openZipFileEntry(new java.io.File(filePath), entryName);
		}
		catch (Exception ex) { }
		return inputStream;
	}

	/**
	 * Safely opens an input stream to an entry within a zip file.
	 * @param file The zip file to be opened.
	 * @param entryName The unique name of the entry within the zip file.
	 * @return Returns an input stream to the zip file's entry.
	 *         <p>
	 *         Returns null if unable to fine the given zip file or its entry.
	 */
	private java.io.InputStream openZipFileEntry(java.io.File file, String entryName) {
		// Validate arguments.
		if ((file == null) || (entryName == null) || (entryName.length() <= 0)) {
			return null;
		}

		// Open a stream to the zip file entry.
		return ZipFileEntryInputStream.tryOpen(file, entryName);
	}

	/**
	 * Safely opens a file via an AssetFileDescriptor without exceptions.
	 * @param filePath The path and name of the file to access.
	 * @return Returns an AssetFileDescriptor to the given file.
	 *         <p>
	 *         Returns null if the file could not be found.
	 */
	public android.content.res.AssetFileDescriptor openAssetFileDescriptorFor(String filePath) {
		// Validate arguments.
		if ((filePath == null) || (filePath.length() <= 0)) {
			return null;
		}

		// Open the file via a descriptor.
		android.content.res.AssetFileDescriptor descriptor = null;
		if (isAssetFile(filePath)) {
			if (descriptor == null) {
				if (descriptor == null) {
					// Attempt to access the file in the APK's "assets" directory.
					try { descriptor = getApplicationContext().getAssets().openFd(filePath); }
					catch (Exception ex) { }
					if (descriptor == null) {
						// Attempt to access the file "coronaResources", where extracted resource are
						try {
							java.io.File file = getCoronaResourcesFile(filePath);
							descriptor = new android.content.res.AssetFileDescriptor(
									android.os.ParcelFileDescriptor.open(file, android.os.ParcelFileDescriptor.MODE_READ_ONLY),
									0, android.content.res.AssetFileDescriptor.UNKNOWN_LENGTH);
						}
						catch (Exception ex) { }
						try {
							// extract file it if was not extracted yet
							if(descriptor == null && extractAssetFile(filePath) != null) {
								java.io.File file = getCoronaResourcesFile(filePath);
								descriptor = new android.content.res.AssetFileDescriptor(
										android.os.ParcelFileDescriptor.open(file, android.os.ParcelFileDescriptor.MODE_READ_ONLY),
										0, android.content.res.AssetFileDescriptor.UNKNOWN_LENGTH);
							}
						}
						catch (Exception ignore) {}
						if (descriptor == null) {
							// Attempt to access the file in the APK's "raw" resource directory.
							try {
								String resourceName = createRawResourceNameForAsset(filePath);
								ResourceServices resourceServices = new ResourceServices(getApplicationContext());
								int resourceId = resourceServices.getRawResourceId(resourceName);
								if (resourceId != ResourceServices.INVALID_RESOURCE_ID) {
									descriptor = resourceServices.getResources().openRawResourceFd(resourceId);
								}
							} catch (Exception ex) {
							}
						}
					}
				}
			}
		}
		else {
			// Attempt to create a descriptor for the external file.
			try {
				java.io.File file = new java.io.File(filePath);
				descriptor = new android.content.res.AssetFileDescriptor(
						android.os.ParcelFileDescriptor.open(file, android.os.ParcelFileDescriptor.MODE_READ_ONLY),
						0, android.content.res.AssetFileDescriptor.UNKNOWN_LENGTH);
			}
			catch (Exception ex) { }
		}
		return descriptor;
	}

	/**
	 * Safely opens a file via an AssetFileDescriptor without exceptions.
	 * @param file The file to access.
	 * @return Returns an AssetFileDescriptor to the given file.
	 *         <p>
	 *         Returns null if the file could not be found.
	 */
	public android.content.res.AssetFileDescriptor openAssetFileDescriptorFor(java.io.File file) {
		// Validate argument.
		if (file == null) {
			return null;
		}

		// Open the file via a descriptor.
		return openAssetFileDescriptorFor(file.getPath());
	}

	/**
	 * Extracts the given asset file to an external directory to make it available to native C/C++ APIs
	 * that do not have access to this application's assets directory within the APK.
	 * The file will be extracted to a hidden folder under the application's directory.
	 * <p>
	 * If the given file has already been extracted, then this method will not attempt to extract it again
	 * unless this application has been re-installed on the device. For the re-installed case, Corona
	 * must assume that this application might have modified assets that need to be re-extracted.
	 * @param filePath Relative path to the asset file within the APK's assets directory.
	 * @return Returns an absolute path to where the asset file was copied to.
	 *         <p>
	 *         Returns null if unable to find the given asset file.
	 */
	public java.io.File extractAssetFile(String filePath) {
		return extractAssetFile(filePath, false);
	}

	/**
	 * Extracts the given asset file to an external directory to make it available to native C/C++ APIs
	 * that do not have access to this application's assets directory within the APK.
	 * The file will be extracted to a hidden folder under the application's directory.
	 * <p>
	 * If the given file has already been extracted, you can tell force the rewrite 
	 * @param filePath Relative path to the asset file within the APK's assets directory.
	 * @param overwrite Whether the file should be overwrite whats already extracted, if anything.  True to overwrite.
	 * @return Returns an absolute path to where the asset file was copied to.
	 *         <p>
	 *         Returns null if unable to find the given asset file.
	 */
	public java.io.File extractAssetFile(String filePath, boolean overwrite) {
		// Validate arguments.
		if ((filePath == null) || (filePath.length() <= 0)) {
			return null;
		}
		// Externalize the given file.
		java.io.File destinationFile = null;
		try {
			destinationFile = extractAssetFile(new java.io.File(filePath), overwrite);
		}
		catch (Exception ex) {
			ex.printStackTrace();
		}
		return destinationFile;
	}

	/**
	 * Extracts the given asset file to an external directory to make it available to native C/C++ APIs
	 * that do not have access to this application's assets directory within the APK.
	 * The file will be extracted to a hidden folder under the application's directory.
	 * <p>
	 * If the given file has already been extracted, then this method will not attempt to extract it again
	 * unless this application has been re-installed on the device. For the re-installed case, Corona
	 * must assume that this application might have modified assets that need to be re-extracted.
	 * @param assetFile The asset file within the APK's assets directory.
	 * @return Returns an absolute path to where the asset file was copied to.
	 *         <p>
	 *         Returns null if unable to find the given asset file.
	 */
	public java.io.File extractAssetFile(java.io.File assetFile) {
		return extractAssetFile(assetFile, false);
	}

	/**
	 * Extracts the given asset file to an external directory to make it available to native C/C++ APIs
	 * that do not have access to this application's assets directory within the APK.
	 * The file will be extracted to a hidden folder under the application's directory.
	 * <p>
	 * If the given file has already been extracted, then this method WILL attempt to extract it again
	 * @param assetFile The asset file within the APK's assets directory.
	 * @param overwrite Whether the file should be overwrite whats already extracted, if anything.  True to overwrite.
	 * @return Returns an absolute path to where the asset file was copied to.
	 *         <p>
	 *         Returns null if unable to find the given asset file.
	 */
	public java.io.File extractAssetFile(java.io.File assetFile, boolean overwrite) {
		// Validate argument.
		if (assetFile == null) {
			return null;
		}

		// Do not continue if the given file is not an asset file within the APK.
		// This means that it is already an external file. Just return a path to it as is.
		if (isAssetFile(assetFile.getPath()) == false) {
			return assetFile;
		}

		java.io.File destinationFile = getCoronaResourcesFile(assetFile.getPath());
		if (destinationFile == null) {
			return null;
		}

		// Do not continue if the given asset file has already been externalized and the overwrite flag is set to false
		if (!overwrite && destinationFile.exists()) {
			return destinationFile;
		}

		// Extract the asset file and copy it to an external directory.
		boolean wasCopied = copyFile(assetFile, destinationFile);
		// There is a 0.3% chance of the file fails to be copied (AssetInputStream#read() returns -1 early),
		// we need a reliable extracted, so try again here.
		if (!wasCopied && !copyFile(assetFile, destinationFile)) {
			return null;
		}

		// Return the path to the externalized asset file.
		return destinationFile;
	}

	/**
	 * Returns File inside "coronaResources" directory
	 * @param assetFile The file to build path with
	 * @return java.io.File pointing to destination file. This would not be null if file does not exist
	 */
	private java.io.File getCoronaResourcesFile(String assetFile) {
		// Fetch the application context.
		android.content.Context context = getApplicationContext();
		if (context == null) {
			return null;
		}

		// Create a destination path to copy the given file to.
		java.io.File destinationFile = new java.io.File(context.getFileStreamPath("coronaResources"), assetFile);
		return destinationFile;
	}

	/**
	 * Extracts the extension from the given file.
	 * @param file The file to extract the extension from.
	 * @return Returns the extension from the given file name, excluding the '.' separator.
	 *         <p>
	 *         Returns an empty string if the given file name does not have an extension.
	 *         <p>
	 *         Returns null if the given argument was null.
	 */
	public String getExtensionFrom(java.io.File file) {
		if (file == null) {
			return null;
		}
		return getExtensionFrom(file.getPath());
	}

	/**
	 * Extracts the extension from the given file name.
	 * @param fileName The path and name of the file to extract the extension from.
	 * @return Returns the extension from the given file name, excluding the '.' separator.
	 *         <p>
	 *         Returns an empty string if the given file name does not have an extension.
	 *         <p>
	 *         Returns null if the given argument was null or an empty string.
	 */
	public String getExtensionFrom(String fileName) {
		// Validate.
		if (fileName == null) {
			return null;
		}

		// Remove any white space from the beginning and end of the file name
		// and then make sure that the given string is not empty.
		fileName = fileName.trim();
		if (fileName.length() <= 0) {
			return null;
		}

		// Return the extension from the given file name.
		int index = fileName.lastIndexOf('.');
		if ((index < 0) || ((index + 1) >= fileName.length())) {
			return "";
		}
		return fileName.substring(index + 1);
	}

	/**
	 * Creates a unique "raw" resource name for the given file.
	 * @param file The file to get a resource name from.
	 * @return Returns the unique name for the given file in this APK's "raw" resource directory.
	 *         <p>
	 *         Returns null if the given argument was null.
	 */
	private String createRawResourceNameForAsset(java.io.File file) {
		if (file == null) {
			return null;
		}
		return createRawResourceNameForAsset(file.getPath());
	}

	/**
	 * Creates a unique "raw" resource name for the given file.
	 * @param fileName The path and name of the file to get a resource name from.
	 * @return Returns the unique name for the given file in this APK's "raw" resource directory.
	 *         <p>
	 *         Returns null if the given argument was null or an empty string.
	 */
	private String createRawResourceNameForAsset(String fileName) {
		// Validate.
		if (fileName == null) {
			return null;
		}

		// Remove any white space from the beginning and end of the file name
		// and then make sure that the given string is not empty.
		String resourceName = fileName.trim();
		if (resourceName.length() <= 0) {
			return null;
		}

		// Convert the file name to lower case.
		resourceName = resourceName.toLowerCase();

		// Remove the file's extension.
		int index = resourceName.lastIndexOf('.');
		if (index <= 0) {
			return null;
		}
		resourceName = resourceName.substring(0, index);

		// Replace invalid resource characters with underscores.
		resourceName = resourceName.replaceAll("[^[a-z][0-9]]", "_");

		// Attach the "corona_asset_" prefix to the file name.
		resourceName = "corona_asset_" + resourceName;

		// Return the resulting resource name.
		return resourceName;
	}

	/**
	 * Copies the given file to the given destination safely without exceptions.
	 * Will overwrite the destination file if it already exists.
	 * @param sourceFilePath The path and file name to be copied. Cannot be null.
	 * @param destinationFilePath The path and file name to copy the source file to. Cannot be null.
	 * @return Returns true if the file copy operation was successful. Returns false if not.
	 */
	public boolean copyFile(String sourceFilePath, String destinationFilePath) {
		boolean hasSucceeded = false;
		
		// Validate arguments.
		if ((sourceFilePath == null) || (sourceFilePath.length() <= 0) ||
		    (destinationFilePath == null) || (destinationFilePath.length() <= 0)) {
			return false;
		}
		
		// Copy the file.
		try {
			hasSucceeded = copyFile(new java.io.File(sourceFilePath), new java.io.File(destinationFilePath));
		}
		catch (Exception ex) {
			ex.printStackTrace();
		}
		return hasSucceeded;
	}
	
	/**
	 * Copies the given file to the given destination safely without exceptions.
	 * <p>
	 * Will overwrite the destination file if it already exists.
	 * <p>
	 * Will create the destination's directories if it does not already exist.
	 * @param sourceFile The file to be copied. Cannot be null.
	 * @param destinationFile The path and file name to copy the source file to. Cannot be null.
	 * @return Returns true if the file copy operation was successful. Returns false if not.
	 */
	public boolean copyFile(java.io.File sourceFile, java.io.File destinationFile) {
		java.io.InputStream inputStream = null;
		java.io.FileOutputStream outputStream = null;
		java.io.File tmpFile = null;
		boolean hasSucceeded = false;
		
		// Validate arguments.
		if ((sourceFile == null) || (destinationFile == null)) {
			return false;
		}
		
		// Do not continue if the source file cannot be found.
		// Note: We only do this check for external files.
		if ((isAssetFile(sourceFile.getPath()) == false) && (sourceFile.exists() == false)) {
			return false;
		}

		// Copy the given file.
		try {
			inputStream = openFile(sourceFile);
			if (inputStream != null) {
				// Create the destination directory tree, if it does not already exist.  Only create it if its actually a resource file.
				destinationFile.getParentFile().mkdirs();

				// Writes and renames temporary files to prevent incomplete copies.
				tmpFile = java.io.File.createTempFile("copy-" + destinationFile.getName() + "-", null, destinationFile.getParentFile());
				outputStream = new java.io.FileOutputStream(tmpFile);
				if (outputStream != null) {
					// openFile() returns AssetInputStream, ZipFileEntryInputStream, and so on,
					// overrides available() to use ZipEntry to provide length.
					final int byteCount = inputStream.available();

					// Used for comparison to check if AssetInputStream#read() returns -1 early.
					int readTotal = 0;

					if (byteCount > 0) {
						// 64KB has better throughput but is *suspected* to increase the probability
						// of AssetInputStream#read() returns -1 early. Use general page size 4KB.
						byte[] byteBuffer = new byte[BUFFER_SIZE_NORMAL];
						int readCount;
						while ((readCount = inputStream.read(byteBuffer)) != -1) {
							outputStream.write(byteBuffer, 0, readCount);
							readTotal += readCount;
						}
						// We need the file to be available immediately.
						// The flush method inherited from OutputStream MAY do nothing, but sync does!
						outputStream.flush();
						outputStream.getFD().sync();
					}
					hasSucceeded = (byteCount == readTotal);
				}
			}
		}
		catch (Exception ex) {
			ex.printStackTrace();
		}
		finally {
			// Close the streams.
			if (inputStream != null) {
				try { inputStream.close(); }
				catch (Exception ex) { }
			}
			if (outputStream != null) {
				try { outputStream.close(); }
				catch (Exception ex) { }
			}
			if (tmpFile != null) {
				if (hasSucceeded) {
					hasSucceeded = false;
					// Success if and only if it is successfully copied and renamed.
					try { hasSucceeded = tmpFile.renameTo(destinationFile); }
					catch (Exception ex) { }
				}
				if (!hasSucceeded) tmpFile.delete();
			}
		}
		return hasSucceeded;
	}
	
	/**
	 * Writes the given input stream's bytes to the destination file safely without exceptions.
	 * <p>
	 * Will overwrite the destination file if it already exists.
	 * <p>
	 * Will create the destination's directories if it does not already exist.
	 * @param inputStream The input stream to copy bytes from. Cannot be null.
	 * @param destinationFile The path and file name to copy the input stream's bytes to. Cannot be null.
	 * @return Returns true if the file write operation was successful. Returns false if not.
	 */
	public boolean writeToFile(java.io.InputStream inputStream, java.io.File destinationFile) {
		java.io.FileOutputStream outputStream = null;
		boolean hasSucceeded = false;
		
		// Validate arguments.
		if ((inputStream == null) || (destinationFile == null)) {
			return false;
		}
		
		// Create the destination directory tree, if it does not already exist.
		destinationFile.getParentFile().mkdirs();

		// Write the given stream's bytes to file.
		try {
			outputStream = new java.io.FileOutputStream(destinationFile);
			if (outputStream != null) {
				final int BUFFER_SIZE = inputStream.available() > BUFFER_SIZE_THRESHOLD ? BUFFER_SIZE_LARGE_IO : BUFFER_SIZE_NORMAL;
				byte[] byteBuffer = new byte[BUFFER_SIZE];
				while (true) {
					int bytesToCopy = BUFFER_SIZE;
					bytesToCopy = inputStream.read(byteBuffer, 0, bytesToCopy);
					if (bytesToCopy > 0) {
						outputStream.write(byteBuffer, 0, bytesToCopy);
					}
					else if (bytesToCopy < 0) {
						break;
					}
				}
				hasSucceeded = true;
			}
		}
		catch (Exception ex) {
			ex.printStackTrace();
		}
		finally {
			// Close the output stream.
			if (outputStream != null) {
				try { outputStream.close(); }
				catch (Exception ex) { }
			}
		}
		return hasSucceeded;
	}
	
	/**
	 * Moves the given file to the given destination safely without exceptions.
	 * <p>
	 * Will overwrite the destination file if it already exists.
	 * <p>
	 * Will create the destination's directories if it does not already exist.
	 * @param sourceFile The file to be moved. Cannot be null.
	 * @param destinationFile The path and file name to move the file to. Cannot be null.
	 *                        Does not have to have the same file name as the source.
	 * @return Returns true if the file was moved successful. Returns false if not.
	 */
	public boolean moveFile(java.io.File sourceFile, java.io.File destinationFile) {
		boolean hasSucceeded = false;
		
		// Validate arguments.
		if ((sourceFile == null) || (destinationFile == null)) {
			return false;
		}
		
		// Do not continue if the source file cannot be found.
		if (sourceFile.exists() == false) {
			return false;
		}
		
		// Attempt to move the file.
		try {
			// First attempt to move the file by renaming it, which has the best performance.
			// This will only succeed if the source and destination are on the same volume.
			hasSucceeded = sourceFile.renameTo(destinationFile);
			
			// If the rename failed, then do a file copy and then delete the original file.
			if (hasSucceeded == false) {
				hasSucceeded = copyFile(sourceFile, destinationFile);
				sourceFile.delete();
			}
		}
		catch (Exception ex) {
			ex.printStackTrace();
		}
		return hasSucceeded;
	}

	/**
	 * Fetches all bytes from the given file.
	 * @param filePath The path and name of the file to read from.
	 *                 <p>
	 *                 Relative paths are assumed to be asset files.
	 * @return Returns all of the bytes from the given file as a byte array.
	 *         <p>
	 *         Returns null if the file was not found or if the application does not have read permission.
	 */
	public byte[] getBytesFromFile(String filePath) {
		java.io.InputStream inputStream = null;
		byte[] bytes = null;

		// Validate.
		if ((filePath == null) || (filePath.length() <= 0)) {
			return null;
		}

		// Copy the given file's bytes to an array.
		try {
			inputStream = openFile(filePath);
			if (inputStream != null) {
				final int byteCount = inputStream.available();
				if (byteCount > 0) {
					bytes = new byte[byteCount];
					final int BUFFER_SIZE = byteCount > BUFFER_SIZE_THRESHOLD ? BUFFER_SIZE_LARGE_IO : BUFFER_SIZE_NORMAL;

					for (int bytesCopied = 0; bytesCopied < byteCount;) {
						int bytesToRead = byteCount - bytesCopied;
						if (bytesToRead > BUFFER_SIZE) {
							bytesToRead = BUFFER_SIZE;
						}
						int readBytes = inputStream.read(bytes, bytesCopied, bytesToRead);
						bytesCopied += readBytes;
					}
				}
			}
		}
		catch (Exception ex) {
			ex.printStackTrace();
		}
		finally {
			if (inputStream != null) {
				try { inputStream.close(); }
				catch (Exception ex) { }
			}
		}
		
		// Return the read bytes, if any.
		return bytes;
	}

	/**
	 * Fetches all bytes from the given file.
	 * @param file The file to read from.
	 *             <p>
	 *             If the file has a relative path, then it is assumed to be an asset file.
	 * @return Returns all of the bytes from the given file as a byte array.
	 *         <p>
	 *         Returns null if the file was not found or if the application does not have read permission.
	 */
	public byte[] getBytesFromFile(java.io.File file) {
		if (file == null) {
			return null;
		}
		return getBytesFromFile(file.getPath());
	}

	/**
	 * Handles requests for the MIME type of the data at the given URI.
	 * Not supported by this class. Calling this method will cause an exception to be thrown.
	 * @param uri The URI to query.
	 * @return Returns a MIME type string for the given URI. Returns null if of an unknown type.
	 */
	public String getMimeTypeFrom(android.net.Uri uri) {
		// Validate argument.
		if (uri == null) {
			return null;
		}

		// Fetch the system's MIME type dictionary.
		android.webkit.MimeTypeMap mimeTypeMap = android.webkit.MimeTypeMap.getSingleton();
		if (mimeTypeMap == null) {
			return null;
		}

		// Return the MIME type for the file extension provided by the given URI.
		String fileExtension = mimeTypeMap.getFileExtensionFromUrl(uri.toString());
		return mimeTypeMap.getMimeTypeFromExtension(fileExtension);
	}
}

void WDB::WriteUDP(unsigned int ofs, std::vector<unsigned int> data) {
    size_t i;
    fd_set readfds;
    struct timeval timeout;
    int status, ms, retry;
    struct sockaddr_in client_addr;
    bool bSuccess;

    if (mDemoMode)
        return;

    if (mDCB)
        return mDCB->WriteUDP(mSlot, ofs, data);

    udpSequenceNumber++;
    std::memcpy(&client_addr, mEthAddrBin, sizeof(client_addr));

    std::vector<unsigned char> writeBuf(8);
    std::vector<unsigned char> readBuf(1600);

    writeBuf[0] = 0x14; // Write32 command
    writeBuf[1] = 0;
    writeBuf[2] = udpSequenceNumber >> 8;
    writeBuf[3] = udpSequenceNumber & 0xFF;

    writeBuf[4] = (ofs >> 24) & 0xFF;
    writeBuf[5] = (ofs >> 16) & 0xFF;
    writeBuf[6] = (ofs >> 8) & 0xFF;
    writeBuf[7] = (ofs >> 0) & 0xFF;

    for (auto &d : data) {
        writeBuf.push_back((d >> 24) & 0xFF);
        writeBuf.push_back((d >> 16) & 0xFF);
        writeBuf.push_back((d >> 8) & 0xFF);
        writeBuf.push_back((d >> 0) & 0xFF);
    }

    auto startTime = WP::usStart();
    ms = mReceiveTimeoutMs;

    for (retry = 0; retry < 10; retry++) {
        // Clear input queue
        do {
            FD_ZERO(&readfds);
            FD_SET(gBinSocket, &readfds);
            timeout.tv_sec = 0;
            timeout.tv_usec = 0;

            do {
                status = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);
            } while (status == -1);

            if (!FD_ISSET(gBinSocket, &readfds))
                break;

            recv(gBinSocket, &readBuf[0], readBuf.size(), 0);
        } while (true);

        // Send request
        i = sendto(gBinSocket,
                   &writeBuf[0],
                   writeBuf.size(),
                   0,
                   (struct sockaddr *) &client_addr,
                   sizeof(client_addr));

        // 📝 Log sent data
        std::ofstream logfile("udp_log.txt", std::ios::app);
        if (logfile.is_open()) {
            logfile << "[SEND] ";
            for (auto c : writeBuf)
                logfile << std::hex << std::setw(2) << std::setfill('0') << (int)c << " ";
            logfile << "\n";
        }

        if (i != writeBuf.size()) {
            if (this->mVerbose)
                std::cout << mWDBName << " send retry " << retry + 1 << std::endl;
            continue;
        }

        if (mReceiveTimeoutMs < 0)
            return;

        // Retrieve reply
        do {
            std::fill(readBuf.begin(), readBuf.end(), 0);

            FD_ZERO(&readfds);
            FD_SET(gBinSocket, &readfds);

            if (retry > 0)
                ms *= 1.3;

            timeout.tv_sec = ms / 1000;
            timeout.tv_usec = (ms % 1000) * 1000;

            do {
                status = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);
            } while (status == -1);

            if (!FD_ISSET(gBinSocket, &readfds))
                break;

            i = recv(gBinSocket, &readBuf[0], readBuf.size(), 0);
            assert(i > 0);

            // 📝 Log received data
            if (logfile.is_open()) {
                logfile << "[RECV] ";
                for (int j = 0; j < i; j++)
                    logfile << std::hex << std::setw(2) << std::setfill('0') << (int)readBuf[j] << " ";
                logfile << "\n";
                logfile.close();
            }

            bSuccess = readBuf[0] == 0x14 &&
                       readBuf[1] == 0x01 &&
                       readBuf[2] == ((udpSequenceNumber >> 8) & 0xFF) &&
                       readBuf[3] == (udpSequenceNumber & 0xFF);
            if (bSuccess)
                return;

        } while (1);

        if (this->mVerbose)
            std::cout << mWDBName << " retry " << retry + 1 << " with " << ms << " ms" << std::endl;
    }

    if (this->mVerbose && retry > 0) {
        std::cout << "Communication to " << mWDBName << " took " <<
                  WP::usSince(startTime) / 1000.0 << " ms" << std::endl;
    }

    throw std::runtime_error(std::string("Error writing binary UDP data to " + mWDBName));
}

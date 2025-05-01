std::string WDB::SendReceiveUDP(std::string str, unsigned char *ethAddr) {
   size_t i;
   fd_set readfds;
   struct timeval timeout;
   int status, ms;
   struct sockaddr_in client_addr;
   char rx_buffer[1600];
   std::string result;

   // --- Logging setup ---
   std::ofstream log("udp_log.txt", std::ios::app);
   auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
   log << "\n\n[==== " << std::ctime(&now) << " ====]\n";

   if (ethAddr)
      std::memcpy(&client_addr, ethAddr, sizeof(client_addr));
   else if (mDCB) {
      std::string cmd = "slot " + std::to_string(mSlot) + " " + str;
      log << "[SEND] " << cmd;
      std::string response = mDCB->SendReceiveUDP(cmd);
      log << "[RECV] " << response << "\n";
      return response;
   } else
      std::memcpy(&client_addr, mEthAddrAscii, sizeof(client_addr));

   if (str.empty())
      str = "\n";

   if (str.back() != '\n')
      str += '\n';

   log << "[SEND] " << str;

   result.clear();
   ms = mReceiveTimeoutMs;

   for (int retry = 0; retry < 10; retry++) {
      // Clear input queue
      do {
         FD_ZERO(&readfds);
         FD_SET(gASCIISocket, &readfds);
         timeout.tv_sec = 0;
         timeout.tv_usec = 0;
         do {
            status = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);
         } while (status == -1);
         if (!FD_ISSET(gASCIISocket, &readfds))
            break;
         recv(gASCIISocket, rx_buffer, sizeof(rx_buffer), 0);
      } while (true);

      // Send request
      i = sendto(gASCIISocket, str.c_str(), str.size(), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
      if (i != str.size()) {
         if (this->mVerbose)
            std::cout << mWDBName << " send retry " << retry + 1 << std::endl;
         continue;
      }

      // Receive response
      do {
         std::memset(rx_buffer, 0, sizeof(rx_buffer));
         FD_ZERO(&readfds);
         FD_SET(gASCIISocket, &readfds);
         if (retry > 0)
            ms *= 1.3;
         timeout.tv_sec = ms / 1000;
         timeout.tv_usec = (ms % 1000) * 1000;
         do {
            status = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);
         } while (status == -1 && errno == EINTR);
         if (!FD_ISSET(gASCIISocket, &readfds))
            break;

         i = recv(gASCIISocket, rx_buffer, sizeof(rx_buffer), 0);
         assert(i > 0);
         result += rx_buffer;

         if (mPrompt == "")
            mPrompt = result;

         if (result.substr(result.size() - mPrompt.size()) == mPrompt)
            break;
      } while (1);

      if (mPrompt.size() > 0 && result.size() >= mPrompt.size() &&
          result.substr(result.size() - mPrompt.size()) == mPrompt)
         break;

      if (this->mVerbose)
         std::cout << mWDBName << " retry " << retry + 1 << " with " << ms << " ms" << std::endl;

      result.clear();
   }

   if (result.size() == 0) {
      if (str.back() == '\n')
         str = str.substr(0, str.size() - 1);
      throw std::runtime_error("Error sending \"" + str + "\" to " + mWDBName);
   }

   // Remove prompt
   if (result.size() >= mPrompt.size())
      result = result.substr(0, result.size() - mPrompt.size());

   log << "[RECV] " << result << "\n";
   log.close();
   return result;
}
